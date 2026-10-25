#pragma once

#include <cstddef>
#include <cstdint>

namespace tradutorlinux::ntdll {

// ---------------------------------------------------------------------------
// Camada NTDLL Unix, que mantém a separação entre APIs Win32 e operações POSIX:
//   kernel32!VirtualAlloc → ntdll!NtAllocateVirtualMemory → mmap
// Isso mantém a fronteira ABI pequena, facilita testes e permite implementar
// MEM_RESERVE e MEM_COMMIT separadamente. A implementação usa chamadas POSIX.
// ---------------------------------------------------------------------------

// NTSTATUS mínimos usados pelo subconjunto suportado. Valores compatíveis
// com winnt.h para facilitar NtStatusToDosError.
enum class NtStatus : std::uint32_t {
    Success = 0x00000000,
    InvalidParameter = 0xC000000D,
    NoMemory = 0xC0000017,
    MemoryNotAllocated = 0xC00000A0,
    ConflictingAddresses = 0xC0000018,
    AccessDenied = 0xC0000022,
};

[[nodiscard]] std::uint32_t NtStatusToDosError(NtStatus status) noexcept;

// Implementa a reserva e o commit de memória virtual de NtAllocateVirtualMemory.
//  - BaseAddress: in/out ponteiro (nullptr para uma nova reserva; endereço exato
//    de uma reserva existente ao fazer MEM_COMMIT separado)
//  - RegionSize: in/out tamanho em bytes (arredondado à página na saída)
//  - AllocationType: MEM_RESERVE, MEM_COMMIT ou ambos (0x2000/0x1000/0x3000)
//  - Protect: PAGE_*
//
// Retorna Success e preenche *BaseAddress/*RegionSize em caso de êxito.
[[nodiscard]] NtStatus NtAllocateVirtualMemory(void** BaseAddress,
                                               std::size_t* RegionSize,
                                               std::uint32_t AllocationType,
                                               std::uint32_t Protect) noexcept;

// Libera região previamente alocada por NtAllocateVirtualMemory.
//  - BaseAddress: in/out base exata retornada por allocate
//  - RegionSize: in/out deve ser 0 quando FreeType == MEM_RELEASE (contrato Fase 5)
//  - FreeType: MEM_RELEASE (0x8000)
[[nodiscard]] NtStatus NtFreeVirtualMemory(void* BaseAddress,
                                           std::size_t* RegionSize,
                                           std::uint32_t FreeType) noexcept;

// Altera a proteção de uma região já mapeada (mprotect).
[[nodiscard]] NtStatus NtProtectVirtualMemory(void* BaseAddress,
                                              std::size_t* RegionSize,
                                              std::uint32_t NewProtect,
                                              std::uint32_t* OldProtect) noexcept;

// Consulta informação da região — wrapper sobre /proc/self/maps + tabela
// de alocações privadas. Espelha NtQueryVirtualMemory usado por VirtualQuery.
struct NtMemoryInformation {
    void* BaseAddress{nullptr};
    void* AllocationBase{nullptr};
    std::uint32_t AllocationProtect{0};
    std::size_t RegionSize{0};
    std::uint32_t State{0};    // MEM_COMMIT etc.
    std::uint32_t Protect{0};  // PAGE_*
    std::uint32_t Type{0};     // MEM_PRIVATE / MEM_IMAGE
};

[[nodiscard]] NtStatus NtQueryVirtualMemory(const void* Address,
                                            NtMemoryInformation* Info) noexcept;

}  // namespace tradutorlinux::ntdll
