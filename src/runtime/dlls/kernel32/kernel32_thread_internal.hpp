#pragma once

#include "kernel32_common.hpp"

#include "../../core/runtime_handle_state.hpp"
#include "../../core/runtime_process_state.hpp"
#include "../../core/runtime_thread_state.hpp"

#include "tradutorlinux/runtime/teb.hpp"
#include "tradutorlinux/runtime/unwind.hpp"

#include <array>
#include <csetjmp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

// Contexto de saída por thread do convidado (definidos em thread.cpp).
// Auditoria A2: apenas a thread dona do jmp_buf pode desempilhar para ele;
// as demais threads usam o próprio jmp_buf (terminate_guest_thread_...).
namespace tradutorlinux {
extern thread_local std::jmp_buf* t_thread_exit_context;
extern thread_local ThreadSlot* t_thread_exit_slot;

// Termina a thread atual: worker com jmp_buf próprio → exit da thread; a
// thread dona do jmp_buf do processo → desempilha o processo inteiro.
void terminate_guest_thread_for_process_exit() noexcept;
}  // namespace tradutorlinux
