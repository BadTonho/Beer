#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <poll.h>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using namespace std::chrono_literals;

struct XvfbProcess {
    pid_t pid{-1};
    std::string display;
};

enum class SmokeResult { Passed, Failed, Skipped };

[[nodiscard]] bool wait_for_exit(const pid_t pid, const std::chrono::milliseconds timeout,
                                  int* const exit_status = nullptr) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        if (::waitpid(pid, &status, WNOHANG) == pid) {
            if (exit_status != nullptr) {
                *exit_status = status;
            }
            return true;
        }
        std::this_thread::sleep_for(20ms);
    }
    return false;
}

void stop_process(const pid_t pid) {
    if (pid <= 0 || wait_for_exit(pid, 100ms)) {
        return;
    }
    ::kill(pid, SIGKILL);
    (void)::waitpid(pid, nullptr, 0);
}

void stop_runtime_process(const pid_t pid) {
    if (pid <= 0 || wait_for_exit(pid, 100ms)) {
        return;
    }
    if (::getpgid(pid) == pid) {
        ::kill(-pid, SIGKILL);
    } else {
        ::kill(pid, SIGKILL);
    }
    (void)::waitpid(pid, nullptr, 0);
}

[[nodiscard]] XvfbProcess start_xvfb() {
    int display_pipe[2]{};
    if (::pipe(display_pipe) != 0) {
        return {};
    }
    const pid_t pid = ::fork();
    if (pid == 0) {
        ::close(display_pipe[0]);
        const int null_fd = ::open("/dev/null", O_WRONLY);
        if (null_fd >= 0) {
            (void)::dup2(null_fd, STDERR_FILENO);
            ::close(null_fd);
        }
        if (::dup2(display_pipe[1], 3) < 0) {
            ::_exit(127);
        }
        ::close(display_pipe[1]);
        ::execlp("Xvfb", "Xvfb", "-displayfd", "3", "-screen", "0", "800x600x24",
                 "-nolisten", "tcp", static_cast<char*>(nullptr));
        ::_exit(127);
    }
    ::close(display_pipe[1]);
    if (pid < 0) {
        ::close(display_pipe[0]);
        return {};
    }

    std::string display_number;
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < deadline && display_number.empty()) {
        struct pollfd descriptor{display_pipe[0], POLLIN | POLLHUP, 0};
        if (::poll(&descriptor, 1, 100) <= 0) {
            continue;
        }
        char character = '\0';
        const ::ssize_t count = ::read(display_pipe[0], &character, 1);
        if (count == 1 && character != '\n') {
            display_number.push_back(character);
        } else if (count <= 0) {
            break;
        }
    }
    ::close(display_pipe[0]);
    if (display_number.empty()) {
        stop_process(pid);
        return {};
    }
    return XvfbProcess{pid, ":" + display_number};
}

[[nodiscard]] std::string to_windows_z_path(const std::filesystem::path& path,
                                            const bool trailing_separator = false) {
    std::string s = "Z:" + path.string();
    for (char& c : s) {
        if (c == '/') {
            c = '\\';
        }
    }
    if (trailing_separator && (s.empty() || s.back() != '\\')) {
        s.push_back('\\');
    }
    return s;
}

struct ProcessOutcome {
    bool launched{false};
    bool exited{false};
    bool timed_out{false};
    int status{0};
};

[[nodiscard]] ProcessOutcome run_process(const std::filesystem::path& runtime,
                                         const std::vector<std::string>& arguments,
                                         const std::filesystem::path& home,
                                         const std::filesystem::path& working_directory,
                                         const std::filesystem::path& log_path,
                                         const std::string& display,
                                         const std::chrono::milliseconds timeout) {
    const int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (log_fd < 0) {
        return {};
    }

    const pid_t pid = ::fork();
    if (pid == 0) {
        (void)::setpgid(0, 0);
        const std::filesystem::path config_home = home / "config";
        const std::filesystem::path data_home = home / "data";
        const std::filesystem::path appdata = home / "appdata";
        if (::setenv("HOME", home.c_str(), 1) != 0 ||
            ::setenv("XDG_CONFIG_HOME", config_home.c_str(), 1) != 0 ||
            ::setenv("XDG_DATA_HOME", data_home.c_str(), 1) != 0 ||
            ::setenv("APPDATA", appdata.c_str(), 1) != 0 ||
            ::setenv("DISPLAY", display.c_str(), 1) != 0 ||
            ::unsetenv("WAYLAND_DISPLAY") != 0 ||
            (!working_directory.empty() && ::chdir(working_directory.c_str()) != 0)) {
            ::_exit(126);
        }
        (void)::dup2(log_fd, STDOUT_FILENO);
        (void)::dup2(log_fd, STDERR_FILENO);
        ::close(log_fd);

        std::vector<char*> child_argv;
        child_argv.reserve(arguments.size() + 2);
        child_argv.push_back(const_cast<char*>(runtime.c_str()));
        for (const std::string& argument : arguments) {
            child_argv.push_back(const_cast<char*>(argument.c_str()));
        }
        child_argv.push_back(nullptr);
        ::execv(runtime.c_str(), child_argv.data());
        ::_exit(127);
    }

    ::close(log_fd);
    if (pid < 0) {
        return {};
    }
    (void)::setpgid(pid, pid);

    ProcessOutcome outcome;
    outcome.launched = true;
    outcome.exited = wait_for_exit(pid, timeout, &outcome.status);
    if (!outcome.exited) {
        outcome.timed_out = true;
        stop_runtime_process(pid);
    }
    return outcome;
}

[[nodiscard]] bool exited_successfully(const ProcessOutcome& result) {
    return result.launched && result.exited && !result.timed_out &&
           WIFEXITED(result.status) && WEXITSTATUS(result.status) == 0;
}

[[nodiscard]] std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] std::string tail_lines(const std::string& contents, const std::size_t maximum_lines) {
    std::size_t end = contents.size();
    if (end > 0 && contents[end - 1] == '\n') --end;
    std::size_t start = end;
    std::size_t lines = 0;
    while (start > 0 && lines < maximum_lines) {
        const std::size_t newline = contents.rfind('\n', start - 1);
        if (newline == std::string::npos) {
            start = 0;
            break;
        }
        start = newline + 1;
        ++lines;
    }
    return contents.substr(start, end - start);
}

[[nodiscard]] SmokeResult run_extraction_smoke(const std::filesystem::path& runtime,
                                              const std::filesystem::path& target) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path staging = std::filesystem::temp_directory_path() /
                                          ("tradutorlinux-winrar-extract-" +
                                           std::to_string(::getpid()) + "-" +
                                           std::to_string(stamp));
    const std::filesystem::path dest = staging / "dest";
    const std::filesystem::path archive_workspace = staging / "archive-work";
    const std::filesystem::path prefix = staging / "prefix";
    const std::filesystem::path home = staging / "home";
    const std::filesystem::path config_home = home / "config";
    const std::filesystem::path data_home = home / "data";
    const std::filesystem::path appdata = home / "appdata";

    std::error_code error;
    std::filesystem::create_directories(dest, error);
    if (!error) std::filesystem::create_directories(archive_workspace, error);
    if (!error) std::filesystem::create_directories(prefix, error);
    if (!error) std::filesystem::create_directories(config_home, error);
    if (!error) std::filesystem::create_directories(data_home, error);
    if (!error) std::filesystem::create_directories(appdata, error);
    if (error) {
        std::cerr << "falha ao criar diretorio de staging: " << error.message() << '\n';
        return SmokeResult::Failed;
    }

    const XvfbProcess xvfb = start_xvfb();
    if (xvfb.pid <= 0 || xvfb.display.empty()) {
        std::filesystem::remove_all(staging, error);
        std::cerr << "smoke de extracao do WinRAR: Xvfb indisponivel; cenario ignorado\n";
        return SmokeResult::Skipped;
    }

    const std::string dest_arg = "-d" + to_windows_z_path(dest, true);
    const auto installer_run = run_process(
        runtime,
        {"--trace=runtime,process,gui", "--timeout", "15", "--cpu", "10", "--memory", "1024",
         target.string(), "--", "-s", dest_arg},
        home, {}, staging / "installer-extraction.log", xvfb.display, 15s);
    const std::string installer_trace = read_text_file(staging / "installer-extraction.log");
    const bool installer_run_ok = exited_successfully(installer_run);
    const bool installer_trace_ok = installer_trace.find(
                                        "ExitProcess symbol=\"ExitProcess\" exit-code=\"0\"") != std::string::npos &&
                                    installer_trace.find("guest-timeout") == std::string::npos &&
                                    installer_trace.find("guest-signal") == std::string::npos;

    // Keep validation of the SFX installer payload separate from the RAR workflow.
    const std::filesystem::path license_file = dest / "License.txt";
    const std::filesystem::path winrar_exe = dest / "WinRAR.exe";
    const std::filesystem::path rar_exe = dest / "Rar.exe";
    const std::filesystem::path unrar_exe = dest / "UnRAR.exe";

    bool installer_files_ok = std::filesystem::is_regular_file(license_file, error) && !error &&
                              std::filesystem::file_size(license_file, error) > 1000 && !error &&
                              std::filesystem::is_regular_file(winrar_exe, error) && !error &&
                              std::filesystem::file_size(winrar_exe, error) > 1000000 && !error &&
                              std::filesystem::is_regular_file(rar_exe, error) && !error &&
                              std::filesystem::file_size(rar_exe, error) > 500000 && !error &&
                              std::filesystem::is_regular_file(unrar_exe, error) && !error &&
                              std::filesystem::file_size(unrar_exe, error) > 100000 && !error;

    if (installer_files_ok) {
        const std::string license_content = read_text_file(license_file);
        if (license_content.find("END USER LICENSE AGREEMENT") == std::string::npos) {
            installer_files_ok = false;
        }
    }

    bool rar_created_ok = false;
    bool catalog_add_ok = false;
    bool unrar_run_ok = false;
    bool extracted_bytes_ok = false;
    bool catalog_remove_ok = false;
    bool catalog_cleanup_ok = false;
    std::string rar_create_log;
    std::string catalog_add_log;
    std::string unrar_run_log;
    std::string catalog_remove_log;
    std::string catalog_list_log;

    const std::filesystem::path input_file = archive_workspace / "arquivo de teste.txt";
    const std::filesystem::path rar_file = archive_workspace / "u4-real.rar";
    const std::filesystem::path extract_dest = archive_workspace / "unpacked";
    const std::string expected_content = "fixture RAR da U4\nconteudo com espacos e acentos: ação, café e 日本語\n";
    bool fixture_written = false;
    {
        std::ofstream fixture(input_file, std::ios::binary);
        fixture.write(expected_content.data(), static_cast<std::streamsize>(expected_content.size()));
        fixture_written = fixture.good();
    }
    std::filesystem::create_directory(extract_dest, error);
    if (!error && fixture_written && installer_run_ok && installer_trace_ok && installer_files_ok) {
        const auto rar_create = run_process(
            runtime,
            {"--trace=runtime,process", "--timeout", "15", "--cpu", "10", "--memory", "1024",
             rar_exe.string(), "a", "-ep1", to_windows_z_path(rar_file),
             to_windows_z_path(input_file)},
            home, dest, staging / "rar-create.log", xvfb.display, 15s);
        rar_create_log = read_text_file(staging / "rar-create.log");
        rar_created_ok = exited_successfully(rar_create) &&
                         rar_create_log.find("ExitProcess symbol=\"ExitProcess\" exit-code=\"0\"") != std::string::npos &&
                         rar_create_log.find("guest-timeout") == std::string::npos &&
                         rar_create_log.find("guest-signal") == std::string::npos &&
                         std::filesystem::is_regular_file(rar_file, error) && !error &&
                         std::filesystem::file_size(rar_file, error) > 64 && !error;
    }

    const std::string app_id = "winrar-unrar-u4";
    if (rar_created_ok) {
        const auto add = run_process(
            runtime,
            {"app", "add", unrar_exe.string(), "--name", "WinRAR UnRAR U4", "--prefix", prefix.string(),
             "--id", app_id},
            home, {}, staging / "catalog-add.log", xvfb.display, 10s);
        catalog_add_log = read_text_file(staging / "catalog-add.log");
        catalog_add_ok = exited_successfully(add);
    }

    if (catalog_add_ok) {
        const auto unrar_run = run_process(
            runtime,
            {"app", "run", app_id, "--trace=runtime,process", "--timeout", "15", "--cpu", "10", "--memory", "1024",
             "x", "-o+", to_windows_z_path(rar_file), to_windows_z_path(extract_dest, true)},
            home, {}, staging / "unrar-extract.log", xvfb.display, 15s);
        unrar_run_log = read_text_file(staging / "unrar-extract.log");
        unrar_run_ok = exited_successfully(unrar_run) &&
                       unrar_run_log.find("ExitProcess symbol=\"ExitProcess\" exit-code=\"0\"") != std::string::npos &&
                       unrar_run_log.find("guest-timeout") == std::string::npos &&
                       unrar_run_log.find("guest-signal") == std::string::npos;
        extracted_bytes_ok = read_file(extract_dest / input_file.filename()) == expected_content;

        const auto remove = run_process(runtime, {"app", "remove", app_id}, home, {},
                                        staging / "catalog-remove.log", xvfb.display, 10s);
        catalog_remove_log = read_text_file(staging / "catalog-remove.log");
        catalog_remove_ok = exited_successfully(remove);
        const auto list = run_process(runtime, {"app", "list"}, home, {},
                                      staging / "catalog-list.log", xvfb.display, 10s);
        catalog_list_log = read_text_file(staging / "catalog-list.log");
        catalog_cleanup_ok = exited_successfully(list) &&
                             catalog_list_log.find("[" + app_id + "]") == std::string::npos &&
                             std::filesystem::is_regular_file(unrar_exe, error) && !error &&
                             std::filesystem::is_directory(prefix, error) && !error;
    }

    stop_process(xvfb.pid);

    const bool passed = installer_run_ok && installer_trace_ok && installer_files_ok &&
                        rar_created_ok && catalog_add_ok && unrar_run_ok && extracted_bytes_ok &&
                        catalog_remove_ok && catalog_cleanup_ok;
    if (!passed) {
        std::cerr << "smoke de fluxos CLI do WinRAR falhou:\n"
                  << "  instalador ExitProcess 0: " << (installer_run_ok && installer_trace_ok ? "true" : "false") << '\n'
                  << "  arquivos do instalador (WinRAR/Rar/UnRAR/licença): " << (installer_files_ok ? "true" : "false") << '\n'
                  << "  criação de RAR real por Rar.exe: " << (rar_created_ok ? "true" : "false") << '\n'
                  << "  cadastro temporário do UnRAR.exe: " << (catalog_add_ok ? "true" : "false") << '\n'
                  << "  extração via app run, exit normal: " << (unrar_run_ok ? "true" : "false") << '\n'
                  << "  bytes extraídos iguais à entrada: " << (extracted_bytes_ok ? "true" : "false") << '\n'
                  << "  remoção do cadastro: " << (catalog_remove_ok ? "true" : "false") << '\n'
                  << "  catálogo limpo e prefixo/executável preservados: " << (catalog_cleanup_ok ? "true" : "false") << '\n';
        const auto print_log = [](const char* label, const std::string& content) {
            if (content.empty()) return;
            const std::string tail = tail_lines(content, 40);
            std::cerr << label << ":\n";
            if (tail.size() < content.size()) std::cerr << "[mostrando as últimas 40 linhas]\n";
            std::cerr << tail << '\n';
        };
        print_log("Instalador", installer_trace);
        print_log("Criação do RAR", rar_create_log);
        print_log("app add", catalog_add_log);
        print_log("app run UnRAR", unrar_run_log);
        print_log("app remove", catalog_remove_log);
        print_log("app list", catalog_list_log);
    }

    std::filesystem::remove_all(staging, error);
    if (passed) {
        std::cout << "WinRAR: instalador SFX conferido; Rar.exe criou um .rar, UnRAR.exe o extraiu via catálogo com bytes idênticos; app remove preservou prefixo e executável\n";
    }
    return passed ? SmokeResult::Passed : SmokeResult::Failed;
}

}  // namespace

int main(const int argc, char** const argv) {
    if (argc != 3) {
        std::cerr << "uso: winrar_extract_smoke <runtime> <WinRAR_x64.exe>\n";
        return 2;
    }
    const std::filesystem::path runtime = argv[1];
    const std::filesystem::path target = argv[2];
    std::error_code error;
    if (!std::filesystem::is_regular_file(runtime, error) || error ||
        !std::filesystem::is_regular_file(target, error) || error) {
        std::cerr << "runtime ou amostra WinRAR inexistente\n";
        return 2;
    }
    const SmokeResult result = run_extraction_smoke(runtime, target);
    return result == SmokeResult::Passed ? 0 : result == SmokeResult::Skipped ? 77 : 1;
}
