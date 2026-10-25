#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <string>
#include <string_view>
#include <thread>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using namespace std::chrono_literals;

struct XvfbProcess {
    pid_t pid{-1};
    std::string display;
};

struct WestonProcess {
    pid_t pid{-1};
    std::string socket;
};

enum class GuiBackend { X11, Wayland };

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
        ::execlp("Xvfb", "Xvfb", "-displayfd", "3", "-screen", "0", "1600x1200x24",
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

[[nodiscard]] WestonProcess start_weston(const std::filesystem::path& runtime_dir,
                                          const std::string& display,
                                          const std::filesystem::path& log_path) {
    constexpr char socket[] = "tl-notepadpp-smoke";
    std::string backend_option = "--backend=x11";
    std::string shell_option = "--shell=desktop";
    std::string module_map;
    const char* const local_module_root = std::getenv("TL_WESTON_MODULE_ROOT");
    const std::filesystem::path config_home = runtime_dir.parent_path() / "weston-config";
    std::error_code config_error;
    std::filesystem::create_directories(config_home, config_error);
    if (config_error) return {};
    {
        std::ofstream config(config_home / "weston.ini");
        config << "[shell]\nbinding-modifier=super\n";
        if (!config.good()) return {};
    }
    if (local_module_root != nullptr && *local_module_root != '\0') {
        const std::filesystem::path root{local_module_root};
        std::error_code iteration_error;
        for (const auto& entry : std::filesystem::directory_iterator(root, iteration_error)) {
            const std::filesystem::path candidate = entry.path() / "x11-backend.so";
            if (entry.is_directory() && entry.path().filename().string().starts_with("libweston-") &&
                std::filesystem::is_regular_file(candidate)) {
                module_map = "x11-backend.so=" + candidate.string();
                break;
            }
        }
        const std::filesystem::path local_shell = root / "weston" / "desktop-shell.so";
        if (std::filesystem::is_regular_file(local_shell)) {
            if (!module_map.empty()) module_map.push_back(';');
            module_map += "desktop-shell.so=" + local_shell.string();
        }
        const std::filesystem::path local_exec = root.parent_path() / "libexec";
        for (const char* const helper : {"weston-desktop-shell", "weston-keyboard"}) {
            const std::filesystem::path helper_path = local_exec / helper;
            if (std::filesystem::is_regular_file(helper_path)) {
                if (!module_map.empty()) module_map.push_back(';');
                module_map += std::string{helper} + "=" + helper_path.string();
            }
        }
        const std::filesystem::path local_data = root.parent_path() / "share" / "weston";
        if (std::filesystem::is_directory(local_data)) {
            (void)::setenv("WESTON_DATA_DIR", local_data.c_str(), 1);
        }
    }
    const pid_t pid = ::fork();
    if (pid == 0) {
        (void)::setenv("DISPLAY", display.c_str(), 1);
        (void)::setenv("XDG_RUNTIME_DIR", runtime_dir.c_str(), 1);
        (void)::setenv("XDG_CONFIG_HOME", config_home.c_str(), 1);
        (void)::setenv("WAYLAND_DISPLAY", socket, 1);
        if (!module_map.empty()) (void)::setenv("WESTON_MODULE_MAP", module_map.c_str(), 1);
        const int log_fd = ::open(log_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (log_fd >= 0) {
            (void)::dup2(log_fd, STDERR_FILENO);
            ::close(log_fd);
        }
        ::execlp("weston", "weston", backend_option.c_str(), "--renderer=pixman",
                 "--width=1600", "--height=1200", "--socket=tl-notepadpp-smoke",
                 shell_option.c_str(),
                 "--idle-time=0", static_cast<char*>(nullptr));
        ::_exit(127);
    }
    if (pid < 0) return {};
    const auto socket_path = runtime_dir / socket;
    const auto deadline = std::chrono::steady_clock::now() + 8s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (std::filesystem::exists(socket_path)) return {pid, socket};
        if (wait_for_exit(pid, 10ms)) return {};
        std::this_thread::sleep_for(20ms);
    }
    stop_process(pid);
    return {};
}

[[nodiscard]] Window find_window_by_name(Display* const display, const Window root,
                                          const std::string& name) {
    const Atom net_wm_name = ::XInternAtom(display, "_NET_WM_NAME", False);
    const Atom utf8_string = ::XInternAtom(display, "UTF8_STRING", False);
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long item_count = 0;
    unsigned long remaining = 0;
    unsigned char* property = nullptr;
    const int property_status = ::XGetWindowProperty(
        display, root, net_wm_name, 0, 4096, False, utf8_string, &actual_type,
        &actual_format, &item_count, &remaining, &property);
    bool matches = property_status == Success && actual_type == utf8_string &&
                   actual_format == 8 && property != nullptr &&
                   std::string_view{reinterpret_cast<const char*>(property), item_count}
                           .find(name) != std::string_view::npos;
    if (property != nullptr) {
        ::XFree(property);
    }
    if (!matches) {
        char* window_name = nullptr;
        if (::XFetchName(display, root, &window_name) != 0 && window_name != nullptr) {
            matches = std::string_view{window_name}.find(name) != std::string_view::npos;
            ::XFree(window_name);
        }
    }
    if (matches) {
        XWindowAttributes attributes{};
        if (::XGetWindowAttributes(display, root, &attributes) != 0 &&
            attributes.map_state == IsViewable) {
            return root;
        }
    }

    Window returned_root = 0;
    Window returned_parent = 0;
    Window* children = nullptr;
    unsigned int child_count = 0;
    if (::XQueryTree(display, root, &returned_root, &returned_parent, &children, &child_count) == 0) {
        return 0;
    }
    Window result = 0;
    for (unsigned int index = 0; index < child_count && result == 0; ++index) {
        result = find_window_by_name(display, children[index], name);
    }
    if (children != nullptr) {
        ::XFree(children);
    }
    return result;
}

void dump_root_windows(Display* const display, const Window root) {
    Window returned_root = 0;
    Window returned_parent = 0;
    Window* children = nullptr;
    unsigned int child_count = 0;
    if (::XQueryTree(display, root, &returned_root, &returned_parent, &children,
                     &child_count) == 0) {
        return;
    }
    for (unsigned int index = 0; index < child_count; ++index) {
        XWindowAttributes attributes{};
        char* name = nullptr;
        const bool have_attributes =
            ::XGetWindowAttributes(display, children[index], &attributes) != 0;
        const bool have_name = ::XFetchName(display, children[index], &name) != 0 &&
                               name != nullptr;
        std::cerr << "janela X11 id=" << children[index]
                  << " map_state=" << (have_attributes ? attributes.map_state : -1)
                  << " title=" << (have_name ? name : "<sem WM_NAME>") << '\n';
        if (name != nullptr) {
            ::XFree(name);
        }
    }
    if (children != nullptr) {
        ::XFree(children);
    }
}

[[nodiscard]] bool send_close(Display* const display, const Window window) {
    const Atom protocols = ::XInternAtom(display, "WM_PROTOCOLS", False);
    const Atom delete_window = ::XInternAtom(display, "WM_DELETE_WINDOW", False);
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = window;
    event.xclient.message_type = protocols;
    event.xclient.format = 32;
    event.xclient.data.l[0] = static_cast<long>(delete_window);
    event.xclient.data.l[1] = CurrentTime;
    const bool sent = ::XSendEvent(display, window, False, NoEventMask, &event) != 0;
    (void)::XFlush(display);
    return sent;
}

[[nodiscard]] bool send_key(Display* const display, Window window, KeySym symbol, bool pressed,
                            unsigned int state = 0, bool use_xtest = false);
[[nodiscard]] bool send_motion(Display* const display, Window window, int x, int y,
                               unsigned int state = 0, bool use_xtest = false);
[[nodiscard]] bool send_button(Display* const display, Window window, int type, int x, int y,
                               unsigned int state = 0, bool use_xtest = false,
                               unsigned int button = Button1);

[[nodiscard]] bool send_notepadpp_exit(Display* const display, const Window window) {
    bool sent = send_key(display, window, XK_Alt_L, true, 0, true);
    sent = send_key(display, window, XK_F4, true, Mod1Mask, true) && sent;
    sent = send_key(display, window, XK_F4, false, Mod1Mask, true) && sent;
    sent = send_key(display, window, XK_Alt_L, false, 0, true) && sent;
    return sent;
}

[[nodiscard]] std::string window_title(Display* const display, const Window window) {
    const Atom net_wm_name = ::XInternAtom(display, "_NET_WM_NAME", False);
    const Atom utf8_string = ::XInternAtom(display, "UTF8_STRING", False);
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long item_count = 0;
    unsigned long remaining = 0;
    unsigned char* property = nullptr;
    const int property_status = ::XGetWindowProperty(
        display, window, net_wm_name, 0, 4096, False, utf8_string, &actual_type,
        &actual_format, &item_count, &remaining, &property);
    if (property_status == Success && actual_type == utf8_string && actual_format == 8 &&
        property != nullptr) {
        std::string title{reinterpret_cast<const char*>(property), item_count};
        ::XFree(property);
        return title;
    }
    if (property != nullptr) ::XFree(property);
    char* legacy = nullptr;
    if (::XFetchName(display, window, &legacy) == 0 || legacy == nullptr) return {};
    std::string title{legacy};
    ::XFree(legacy);
    return title;
}

[[nodiscard]] bool resize_x11_window(Display* const display, const Window window,
                                      const int width, const int height) {
    ::XResizeWindow(display, window, static_cast<unsigned int>(width),
                    static_cast<unsigned int>(height));
    ::XSync(display, False);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        XWindowAttributes attributes{};
        if (::XGetWindowAttributes(display, window, &attributes) != 0 &&
            attributes.width == width && attributes.height == height) return true;
        std::this_thread::sleep_for(20ms);
    }
    return false;
}

[[nodiscard]] bool resize_wayland_window(Display* const display, const Window weston_window) {
    // The isolated Weston config sets Super as its shell modifier, so Super+Shift+F
    // toggles fullscreen and sends a compositor-driven xdg_toplevel.configure.
    bool sent = send_key(display, weston_window, XK_Super_L, true, 0, true);
    sent = send_key(display, weston_window, XK_Shift_L, true, Mod4Mask, true) && sent;
    sent = send_key(display, weston_window, XK_f, true, Mod4Mask | ShiftMask, true) && sent;
    sent = send_key(display, weston_window, XK_f, false, Mod4Mask | ShiftMask, true) && sent;
    sent = send_key(display, weston_window, XK_Shift_L, false, Mod4Mask, true) && sent;
    sent = send_key(display, weston_window, XK_Super_L, false, 0, true) && sent;
    std::this_thread::sleep_for(500ms);
    return sent;
}

[[nodiscard]] bool wait_for_trace(const std::filesystem::path& trace_path,
                                  const std::string_view text,
                                  const std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        std::ifstream input(trace_path);
        const std::string trace{std::istreambuf_iterator<char>{input}, {}};
        if (trace.find(text) != std::string::npos) return true;
        std::this_thread::sleep_for(25ms);
    }
    return false;
}

[[nodiscard]] Window find_largest_viewable_window(Display* const display, const Window root) {
    Window returned_root = 0;
    Window returned_parent = 0;
    Window* children = nullptr;
    unsigned int child_count = 0;
    if (::XQueryTree(display, root, &returned_root, &returned_parent, &children,
                     &child_count) == 0) return 0;
    Window result = 0;
    unsigned long largest_area = 0;
    for (unsigned int index = 0; index < child_count; ++index) {
        XWindowAttributes attributes{};
        if (::XGetWindowAttributes(display, children[index], &attributes) != 0 &&
            attributes.map_state == IsViewable && attributes.width > 100 &&
            attributes.height > 100) {
            const auto area = static_cast<unsigned long>(attributes.width) *
                              static_cast<unsigned long>(attributes.height);
            if (area > largest_area) {
                result = children[index];
                largest_area = area;
            }
        }
        const Window nested = find_largest_viewable_window(display, children[index]);
        if (nested != 0) {
            XWindowAttributes nested_attributes{};
            if (::XGetWindowAttributes(display, nested, &nested_attributes) != 0) {
                const auto area = static_cast<unsigned long>(nested_attributes.width) *
                                  static_cast<unsigned long>(nested_attributes.height);
                if (area > largest_area) {
                    result = nested;
                    largest_area = area;
                }
            }
        }
    }
    if (children != nullptr) ::XFree(children);
    return result;
}

[[nodiscard]] bool send_key(Display* const display, const Window window, const KeySym symbol,
                             const bool pressed, const unsigned int state,
                             const bool use_xtest) {
    const KeyCode keycode = XKeysymToKeycode(display, symbol);
    if (use_xtest) {
        const bool sent = keycode != 0 &&
            ::XTestFakeKeyEvent(display, keycode, pressed ? True : False, CurrentTime) != 0;
        (void)::XFlush(display);
        std::this_thread::sleep_for(20ms);
        return sent;
    }
    XEvent event{};
    event.xkey.type = pressed ? KeyPress : KeyRelease;
    event.xkey.display = display;
    event.xkey.window = window;
    event.xkey.root = DefaultRootWindow(display);
    event.xkey.subwindow = None;
    event.xkey.time = CurrentTime;
    event.xkey.x = 10;
    event.xkey.y = 10;
    event.xkey.x_root = 10;
    event.xkey.y_root = 10;
    event.xkey.state = state;
    event.xkey.keycode = keycode;
    event.xkey.same_screen = True;
    const bool sent = ::XSendEvent(display, window, True,
        pressed ? KeyPressMask : KeyReleaseMask, &event) != 0;
    (void)::XFlush(display);
    std::this_thread::sleep_for(20ms);
    return sent && event.xkey.keycode != 0;
}

[[nodiscard]] bool send_motion(Display* const display, const Window window, const int x,
                               const int y, const unsigned int state,
                               const bool use_xtest) {
    if (use_xtest) {
        int root_x = 0;
        int root_y = 0;
        Window child = 0;
        const bool translated = ::XTranslateCoordinates(
            display, window, DefaultRootWindow(display), x, y, &root_x, &root_y, &child) != 0;
        const bool sent = translated && ::XTestFakeMotionEvent(
            display, DefaultScreen(display), root_x, root_y, CurrentTime) != 0;
        (void)::XFlush(display);
        std::this_thread::sleep_for(40ms);
        return sent;
    }
    XEvent event{};
    event.xmotion.type = MotionNotify;
    event.xmotion.display = display;
    event.xmotion.window = window;
    event.xmotion.root = DefaultRootWindow(display);
    event.xmotion.time = CurrentTime;
    event.xmotion.x = x;
    event.xmotion.y = y;
    event.xmotion.x_root = x;
    event.xmotion.y_root = y;
    event.xmotion.state = state;
    event.xmotion.same_screen = True;
    const bool sent = ::XSendEvent(display, window, True, PointerMotionMask, &event) != 0;
    (void)::XFlush(display);
    std::this_thread::sleep_for(40ms);
    return sent;
}

[[nodiscard]] bool send_button(Display* const display, const Window window, const int type,
                               const int x, const int y, const unsigned int state,
                               const bool use_xtest, const unsigned int button) {
    if (use_xtest) {
        int root_x = 0;
        int root_y = 0;
        Window child = 0;
        const bool translated = ::XTranslateCoordinates(
            display, window, DefaultRootWindow(display), x, y, &root_x, &root_y, &child) != 0;
        const bool moved = translated && ::XTestFakeMotionEvent(
            display, DefaultScreen(display), root_x, root_y, CurrentTime) != 0;
        const bool clicked = ::XTestFakeButtonEvent(
            display, button, type == ButtonPress ? True : False, CurrentTime) != 0;
        (void)::XFlush(display);
        std::this_thread::sleep_for(40ms);
        return moved && clicked;
    }
    XEvent event{};
    event.xbutton.type = type;
    event.xbutton.display = display;
    event.xbutton.window = window;
    event.xbutton.root = DefaultRootWindow(display);
    event.xbutton.time = CurrentTime;
    event.xbutton.x = x;
    event.xbutton.y = y;
    event.xbutton.x_root = x;
    event.xbutton.y_root = y;
    event.xbutton.state = state;
    event.xbutton.button = button;
    event.xbutton.same_screen = True;
    const bool sent = ::XSendEvent(display, window, True,
                                   type == ButtonPress ? ButtonPressMask : ButtonReleaseMask,
                                   &event) != 0;
    (void)::XFlush(display);
    std::this_thread::sleep_for(40ms);
    return sent;
}

[[nodiscard]] bool click_editor(Display* const display, const Window window,
                                const int x = 400, const int y = 300,
                                const bool use_xtest = false) {
    if (use_xtest) {
        const bool moved = send_motion(display, window, x, y, 0, true);
        const bool pressed = send_button(display, window, ButtonPress, x, y, 0, true);
        const bool released = send_button(display, window, ButtonRelease, x, y, 0, true);
        return moved && pressed && released;
    }
    bool sent = true;
    for (const int type : {ButtonPress, ButtonRelease}) {
        XEvent event{};
        event.xbutton.type = type;
        event.xbutton.display = display;
        event.xbutton.window = window;
        event.xbutton.root = DefaultRootWindow(display);
        event.xbutton.time = CurrentTime;
        event.xbutton.x = x;
        event.xbutton.y = y;
        event.xbutton.x_root = x;
        event.xbutton.y_root = y;
        event.xbutton.button = Button1;
        event.xbutton.same_screen = True;
        sent = ::XSendEvent(display, window, True,
                            type == ButtonPress ? ButtonPressMask : ButtonReleaseMask,
                            &event) != 0 && sent;
        (void)::XFlush(display);
        std::this_thread::sleep_for(20ms);
    }
    return sent;
}

[[nodiscard]] bool replace_document_and_save(Display* const display, const Window window,
                                              const int x, const int y,
                                              const bool use_xtest) {
    (void)::XSetInputFocus(display, window, RevertToParent, CurrentTime);
    (void)::XFlush(display);
    bool sent = click_editor(display, window, x, y, use_xtest);
    sent = send_key(display, window, XK_Control_L, true, 0, use_xtest) && sent;
    sent = send_key(display, window, XK_a, true, ControlMask, use_xtest) && sent;
    sent = send_key(display, window, XK_a, false, ControlMask, use_xtest) && sent;
    sent = send_key(display, window, XK_Control_L, false, 0, use_xtest) && sent;
    const std::string replacement = "salvo pelo u3";
    for (const char character : replacement) {
        KeySym symbol = NoSymbol;
        if (character >= 'a' && character <= 'z')
            symbol = static_cast<KeySym>(XK_a + (character - 'a'));
        else if (character >= '0' && character <= '9')
            symbol = static_cast<KeySym>(XK_0 + (character - '0'));
        else if (character == ' ') symbol = XK_space;
        if (symbol == NoSymbol ||
            !send_key(display, window, symbol, true, 0, use_xtest) ||
            !send_key(display, window, symbol, false, 0, use_xtest)) return false;
    }
    sent = send_key(display, window, XK_Control_L, true, 0, use_xtest) && sent;
    sent = send_key(display, window, XK_s, true, ControlMask, use_xtest) && sent;
    sent = send_key(display, window, XK_s, false, ControlMask, use_xtest) && sent;
    sent = send_key(display, window, XK_Control_L, false, 0, use_xtest) && sent;
    return sent;
}

[[nodiscard]] SmokeResult run_smoke(const std::filesystem::path& runtime,
                                    const std::filesystem::path& target,
                                    const GuiBackend backend) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path staging = std::filesystem::temp_directory_path() /
                                          ("tradutorlinux-notepadpp-smoke-" +
                                           std::to_string(::getpid()) + "-" +
                                           std::to_string(stamp));
    std::error_code error;
    std::filesystem::create_directories(staging, error);
    if (error) {
        return SmokeResult::Failed;
    }

    const std::filesystem::path application_directory = staging / "application";
    std::filesystem::copy(target.parent_path(), application_directory,
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::copy_symlinks,
                          error);
    if (error) {
        std::filesystem::remove_all(staging, error);
        return SmokeResult::Failed;
    }
    const std::filesystem::path staged_target = application_directory / target.filename();

    const std::filesystem::path config_home = staging / "config";
    const std::filesystem::path data_home = staging / "data";
    const std::filesystem::path prefix_root = staging / "prefix";
    const std::filesystem::path file_path = staging / "arquivo de teste ação.txt";
    std::filesystem::create_directories(config_home / "beer", error);
    if (error) return SmokeResult::Failed;
    std::filesystem::create_directories(data_home, error);
    if (error) return SmokeResult::Failed;
    {
        std::ofstream input(file_path);
        input << "conteúdo original\n";
        if (!input.good()) return SmokeResult::Failed;
    }
    {
        std::ofstream catalog(config_home / "beer" / "library.json");
        catalog << "{\"version\":1,\"apps\":[{\"id\":\"notepadpp\","
                << "\"name\":\"Notepad++\",\"executable_path\":\"" << staged_target.string()
                << "\",\"prefix_path\":\"" << prefix_root.string()
                << "\",\"working_directory\":\"" << application_directory.string()
                << "\",\"args\":[]}] }\n";
        if (!catalog.good()) return SmokeResult::Failed;
    }

    const XvfbProcess xvfb = start_xvfb();
    if (xvfb.pid <= 0 || xvfb.display.empty()) {
        std::filesystem::remove_all(staging, error);
        std::cerr << "smoke do Notepad++: Xvfb indisponível; cenário ignorado\n";
        return SmokeResult::Skipped;
    }

    const std::filesystem::path runtime_dir = staging / "wayland-runtime";
    if (backend == GuiBackend::Wayland) {
        std::filesystem::create_directories(runtime_dir, error);
        if (error || ::chmod(runtime_dir.c_str(), 0700) != 0) {
            stop_process(xvfb.pid);
            std::filesystem::remove_all(staging, error);
            return SmokeResult::Failed;
        }
    }
    const WestonProcess weston = backend == GuiBackend::Wayland
        ? start_weston(runtime_dir, xvfb.display, staging / "weston.log")
        : WestonProcess{};
    if (backend == GuiBackend::Wayland && weston.pid <= 0) {
        stop_process(xvfb.pid);
        std::ifstream weston_log(staging / "weston.log");
        std::cerr << weston_log.rdbuf();
        std::filesystem::remove_all(staging, error);
        std::cerr << "smoke Wayland do Notepad++: Weston não iniciou\n";
        return SmokeResult::Failed;
    }

    const std::filesystem::path trace_path = staging / "trace.log";
    const int trace_fd = ::open(trace_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    const pid_t runtime_pid = trace_fd < 0 ? -1 : ::fork();
    if (runtime_pid == 0) {
        (void)::setpgid(0, 0);
        (void)::setenv("DISPLAY", xvfb.display.c_str(), 1);
        (void)::setenv("BEER_GUI_BACKEND", backend == GuiBackend::Wayland ? "wayland" : "x11", 1);
        if (backend == GuiBackend::Wayland) {
            (void)::setenv("WAYLAND_DISPLAY", weston.socket.c_str(), 1);
            (void)::setenv("XDG_RUNTIME_DIR", runtime_dir.c_str(), 1);
        } else {
            (void)::unsetenv("WAYLAND_DISPLAY");
            (void)::unsetenv("XDG_RUNTIME_DIR");
        }
        (void)::setenv("HOME", staging.c_str(), 1);
        (void)::setenv("XDG_CONFIG_HOME", config_home.c_str(), 1);
        (void)::setenv("XDG_DATA_HOME", data_home.c_str(), 1);
        (void)::chdir(application_directory.c_str());
        (void)::dup2(trace_fd, STDERR_FILENO);
        ::close(trace_fd);
        ::execl(runtime.c_str(), runtime.c_str(), "app", "run", "notepadpp",
                "--trace", "--timeout", "20", "--cpu", "8", "--memory", "512",
                "--", file_path.c_str(),
                static_cast<char*>(nullptr));
        ::_exit(127);
    }
    if (trace_fd >= 0) {
        ::close(trace_fd);
    }
    if (runtime_pid < 0) {
        stop_process(xvfb.pid);
        std::filesystem::remove_all(staging, error);
        return SmokeResult::Failed;
    }
    (void)::setpgid(runtime_pid, runtime_pid);

    Display* const display = ::XOpenDisplay(xvfb.display.c_str());
    Window window = 0;
    if (display != nullptr) {
        for (int attempt = 0; attempt < 100 && window == 0; ++attempt) {
            window = backend == GuiBackend::Wayland
                ? find_largest_viewable_window(display, DefaultRootWindow(display))
                : find_window_by_name(display, DefaultRootWindow(display),
                                      file_path.filename().string());
            if (window == 0) {
                std::this_thread::sleep_for(100ms);
            }
        }
        if (window == 0 && backend == GuiBackend::X11) {
            window = find_window_by_name(display, DefaultRootWindow(display), "Notepad++");
        }
        if (window == 0) {
            dump_root_windows(display, DefaultRootWindow(display));
        }
    }

    bool passed = display != nullptr;
    bool file_saved_by_application = false;
    bool resized = false;
    std::string observed_title;
    if (window != 0 && backend == GuiBackend::Wayland) {
        const bool window_created = wait_for_trace(trace_path,
            "backend=wayland window-created caption=", 10s);
        const bool window_mapped = window_created && wait_for_trace(
            trace_path, "backend=wayland window-mapped", 5s);
        passed = passed && window_created && window_mapped;
    }
    const std::string_view expected_resize = backend == GuiBackend::Wayland
        ? "window-resized width=\"1600\" height=\"1200\""
        : "window-resized width=\"1000\" height=\"720\"";
    if (window != 0) {
        if (backend == GuiBackend::Wayland) {
            resized = resize_wayland_window(display, window) &&
                      wait_for_trace(trace_path, expected_resize, 4s);
            (void)::XSetInputFocus(display, window, RevertToParent, CurrentTime);
            file_saved_by_application = replace_document_and_save(display, window, 800, 600,
                                                                   true);
        } else {
            resized = resize_x11_window(display, window, 1000, 720);
            observed_title = window_title(display, window);
            file_saved_by_application = replace_document_and_save(display, window, 500, 360,
                                                                   false);
        }
        std::this_thread::sleep_for(500ms);
        const bool closed = backend == GuiBackend::Wayland
            ? send_notepadpp_exit(display, window)
            : send_close(display, window);
        passed = closed && file_saved_by_application && resized;
    }
    Window resource_error = 0;
    if (display != nullptr) {
        for (int attempt = 0; attempt < 40 && resource_error == 0; ++attempt) {
            resource_error = find_window_by_name(display, DefaultRootWindow(display),
                                                 "Load stylers.xml failed");
            if (resource_error == 0) {
                std::this_thread::sleep_for(100ms);
            }
        }
        if (resource_error != 0) {
            passed = send_close(display, resource_error) && passed;
        }
        ::XCloseDisplay(display);
    }

    int runtime_status = 0;
    const bool runtime_exited = wait_for_exit(runtime_pid, 20000ms, &runtime_status);
    if (!runtime_exited) {
        stop_runtime_process(runtime_pid);
    }
    const bool normal_gui_exit = runtime_exited && WIFEXITED(runtime_status) &&
                                 WEXITSTATUS(runtime_status) == 0;
    if (weston.pid > 0) stop_process(weston.pid);
    stop_process(xvfb.pid);

    std::ifstream trace_input(trace_path);
    const std::string trace{std::istreambuf_iterator<char>{trace_input}, {}};
    std::ifstream saved_input(file_path);
    const std::string saved_contents{std::istreambuf_iterator<char>{saved_input}, {}};
    const bool saved_to_original = saved_contents == "salvo pelo u3" ||
                                   saved_contents == "salvo pelo u3\n";

    const bool title_verified = backend == GuiBackend::Wayland
        ? trace.find("backend=wayland window-created caption=") != std::string::npos &&
          trace.find("backend=wayland window-title caption=") != std::string::npos &&
          trace.find(file_path.filename().string()) != std::string::npos
        : observed_title.find("Notepad++") != std::string::npos ||
          observed_title.find(file_path.filename().string()) != std::string::npos;
    const bool resize_verified = trace.find(expected_resize) != std::string::npos;
    const bool wayland_system_key_verified = backend != GuiBackend::Wayland ||
        trace.find("DispatchMessageA symbol=\"DispatchMessageA\" message=\"260\" phase=\"call\"") !=
            std::string::npos;

    passed = passed && window != 0 && normal_gui_exit && saved_to_original &&
             title_verified && resize_verified && wayland_system_key_verified &&
             trace.find("caption=\"Load stylers.xml failed\"") == std::string::npos &&
             trace.find("guest-signal") == std::string::npos &&
             trace.find("guest-timeout") == std::string::npos;
    if (!passed) {
        std::cerr << "smoke do Notepad++ falhou; window=" << window
                  << " runtime_exited=" << runtime_exited
                  << " exit_status=" << (runtime_exited ? WEXITSTATUS(runtime_status) : -1)
                  << " saved=" << saved_to_original << " resized=" << resized
                  << " title=" << title_verified << " resize-trace=" << resize_verified
                  << " syskey=" << wayland_system_key_verified
                  << " title-value=\"" << observed_title << "\"\n";
        constexpr std::size_t kFailureTraceTailBytes = 24000;
        const std::size_t trace_offset = trace.size() > kFailureTraceTailBytes
            ? trace.size() - kFailureTraceTailBytes : 0;
        std::cerr << "trace tail:\n" << trace.substr(trace_offset);
    }
    std::filesystem::remove_all(staging, error);
    return passed ? SmokeResult::Passed : SmokeResult::Failed;
}

}  // namespace

int main(const int argc, char** const argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "uso: notepadpp_smoke <runtime> <notepad++.exe> [--wayland]\n";
        return 2;
    }
    const GuiBackend backend = argc == 4 && std::string_view{argv[3]} == "--wayland"
        ? GuiBackend::Wayland : GuiBackend::X11;
    if (argc == 4 && backend != GuiBackend::Wayland) {
        std::cerr << "opção de backend inválida\n";
        return 2;
    }
    const std::filesystem::path runtime = argv[1];
    const std::filesystem::path target = argv[2];
    std::error_code error;
    if (!std::filesystem::is_regular_file(runtime, error) || error ||
        !std::filesystem::is_regular_file(target, error) || error) {
        std::cerr << "runtime ou amostra Notepad++ inexistente\n";
        return 2;
    }
    const std::filesystem::path absolute_runtime = std::filesystem::absolute(runtime, error);
    if (error) {
        std::cerr << "não foi possível normalizar o caminho do runtime\n";
        return 2;
    }
    const SmokeResult result = run_smoke(absolute_runtime, target, backend);
    return result == SmokeResult::Passed ? 0 : result == SmokeResult::Skipped ? 77 : 1;
}
