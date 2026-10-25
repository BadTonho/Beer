#pragma once

#include "tradutorlinux/cli.hpp"
#include "tradutorlinux/loader/import_resolver.hpp"
#include "tradutorlinux/loader/process.hpp"
#include "tradutorlinux/pe/pe_reader.hpp"

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <span>
#include <string>
#include <string_view>

namespace tradutorlinux::cli_detail {

constexpr std::string_view kUsage =
    "Uso:\n"
    "  beer [--trace[=canais]] [--report] [--timeout <segundos>] [--cpu <segundos>] [--memory <MiB>] <arquivo.exe> [argumentos...]\n"
    "  beer install <setup.exe|package.msix> [--name <Nome>] [--prefix <dir>] [--app-exe <caminho>] [--cpu <segundos>] [--memory <MiB>] [argumentos...]\n"
    "  beer app list\n"
    "  beer app run <id_ou_nome> [opções] [argumentos...] [-- <arquivo>...]\n"
    "  beer app add <arquivo.exe> [--name <Nome>] [--prefix <dir>] [--id <id>] [--cpu <segundos>] [--memory <MiB>]\n"
    "  beer app remove <id>\n"
    "  beer app associate <id> <add|remove> <mime-type>\n";

[[nodiscard]] const char* status_label(pe::ParseStatus status);
void write_pe_trace(std::ostream& stream, const pe::PeInfo& info,
                    std::string_view backend = {});
void print_pe_summary(std::ostream& stream, const pe::PeInfo& info);
void write_map_trace(std::ostream& stream, const loader::MappedImage& image);
void write_unmap_trace(std::ostream& stream, std::uint64_t base);
void write_map_failed_trace(std::ostream& stream, std::string_view status,
                            const std::string& detail);
void print_map_summary(std::ostream& stream, const loader::MappedImage& image);
void write_imports_trace(std::ostream& stream, const loader::ResolveResult& imports);
void print_imports_summary(std::ostream& stream, const loader::ResolveResult& imports);
[[nodiscard]] loader::ResolveResult print_support_report(
    std::ostream& stream,
    const pe::PeInfo& info,
    std::span<const std::byte> file_bytes = {},
    bool json_output = false,
    const std::filesystem::path& executable_path = {});
[[nodiscard]] ExitCode run_doctor(
    std::ostream& stdout_stream,
    std::ostream& stderr_stream,
    bool json_output);

}  // namespace tradutorlinux::cli_detail
