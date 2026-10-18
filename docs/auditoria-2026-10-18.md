# Auditoria de erros, falhas e incompatibilidades — 2026-10-18

## 1. Contexto e método

- Análise **100% estática** (leitura de código): nenhum build foi executado,
  conforme a regra de custo de build da máquina do usuário.
- Cinco frentes paralelas de auditoria:
  1. pipeline PE (parser → mapper → imagem → imports);
  2. memória convidada, pontes de ABI e unwinding/SEH;
  3. implementações de APIs Win32 (`src/runtime/dlls/**`);
  4. fronteira FFI Rust ↔ C++;
  5. build, CI, suíte de testes e consistência de documentação.
- Os achados mais graves (A1, A3, A2 e o item de CI) foram **verificados
  pessoalmente** por leitura direta antes de serem registrados here (marcados
  com ✅).
- A suíte de testes não foi executada. O preset `debug` tem **1.016 testes**
  registrados e o wiring de testes está correto (nenhum alvo órfão).

## 2. Sumário executivo

O núcleo estrutural do projeto está sólido: o pipeline PE não apresentou
corrupção de memória reproduzível nas invariâncias lidas, a política W^X é
mantida em todos os caminhos, as primitivas de memória convidada são usadas na
quase totalidade dos consumidores e a FFI Rust tem `catch_unwind` em todas as
exportações.

Os problemas concentrados são:

1. **3 bugs críticos capazes de derrubar o processo hospedeiro** (seção 3);
2. **Classe sistêmica de falsa declaração de suporte**: dezenas de exports
   no-op ou com dados falsificados registradas como `ExportSupport::Full`
   (seção 4) — viola a regra do projeto de não declarar suporte sem fixture e
   registro na matriz;
3. **DoS e corridas** em `--report`, `CreateProcess`, `ws2_32` e estado global
   do runtime (seção 6);
4. Divergências de build/CI/docs (seção 7) e gaps de FFI (seção 8).

## 3. A1–A3 — Bugs críticos (derrubam o processo; corrigir primeiro)

### A1 ✅ verificado — `HeapFree` libera o ponteiro errado quando o canário é corrompido

- **Local:** `src/runtime/dlls/kernel32/memory.cpp:242-275`
- **Fato:** a base de `malloc` é `user_ptr - 48` (`kHeapHeaderSize`). Em
  `tl_HeapFree`, quando `get_heap_header()` retorna `nullptr` porque o convidado
  corrompeu `magic`/`canary` por overrun de buffer, o ramo `else` executa
  `std::free(memory)` — e `memory` é `user_ptr` (base + 48), **não a base**.
- **Impacto:** glibc detecta ponteiro inválido e aborta o **processo hospedeiro
  inteiro** (o guest executa no mesmo processo). O mecanismo de canário, que
  devia produzir diagnóstico controlado, vira detonador.
- **Fix mínimo:** guardar/derivar a base de forma confiável (ex.: campo `base`
  no header ou entrada em `g_heap_blocks`) e liberar a base; ou marcar o slot
  como corrompido sem liberar, com trace de módulo/símbolo.
- **Teste:** fixture que corrompe deliberadamente o canário e exige retorno
  `0` + `ERROR_INVALID_PARAMETER` + processo vivo.

### A2 — `ExitProcess` é `longjmp` para o `jmp_buf` da thread principal

- **Local:** `src/runtime/dlls/kernel32/process.cpp:242` (✅ verificado: a linha
  executa `guest_longjmp(g_guest_exit_context, 1)`) e
  `src/runtime/dlls/kernel32/thread.cpp:312-322` (fallback cross-thread
  `ExitThread → tl_ExitProcess`).
- **Fato:** `g_guest_exit_context` é capturado na thread principal. Um worker
  convidado (rede/IO) que chama `ExitProcess` faz `longjmp` de volta a um
  stack frame que não existe nessa thread = comportamento indefinido/crash.
- **Fix mínimo:** em thread secundária, apenas setar o flag de saída + despertar
  as esperas (CV/eventos), deixando a thread **principal** executar o
  `longjmp` no ponto de acesso apropriado; remover o fallback cross-thread.
- **Teste:** fixture de thread convidada que chama `ExitProcess(3)` a partir do
  worker com resultado determinístico.

### A3 ✅ verificado — `SubmitThreadpoolWork` executa callback do convidado com ABI errada, sem TEB e sem stack convidada

- **Local:** `src/runtime/dlls/kernel32/thread.cpp:495-503`;
  registro como `Full` em `src/loader/module.cpp:174` (conforme auditoria).
- **Fato:**
  ```cpp
  std::thread([cb = w->callback, ctx = w->context]() {
      using CallbackFn = void (*)(void*, void*, void*);
      reinterpret_cast<CallbackFn>(cb)(nullptr, ctx, nullptr);
  }).detach();
  ```
  O callback convidado lê os argumentos em **RCX/RDX/R8** (Microsoft x64), mas a
  chamada compilada como função C SysV os passa em RDI/RSI/RDX. Além disso, a
  thread nativa nova não recebe `set_guest_gs_base`, `GuestContextScope`,
  `g_current_thread_id` nem stack convidada — qualquer acesso `gs:`-relativo do
  convidado → SIGSEGV.
- **Impacto:** crash determinístico ou execução com argumentos trocados;
  declaração `Full` sem fixture (viola AGENTS.md).
- **Fix mínimo:** validar `is_guest_executable_address(cb)`, usar `TL_MSABI`
  no tipo e executar via `tl_call_guest_thread_on_stack` com stack/TEB
  próprios (como `CreateThread` faz em `thread.cpp:249-296`); enquanto não
  houver fixture, rebaixar para `Stub` com trace.
- **Relacionado:** `CreateThreadpoolTimer`/`SetThreadpoolTimer` são `Full`, mas
  os timers **nunca disparam** (`tl_SetThreadpoolTimer` só faz `(void)` em
  `thread.cpp:529-534`).

## 4. Falsa declaração de suporte (`ExportSupport::Full` para stubs)

Padrão sistêmico: `src/loader/module.cpp` (dlls/user32) declara **335 exports
como `ExportSupport::Full`**, incluindo dezenas de no-ops ou dados falsos.
Como o mecanismo de rastreio/diagnóstico é exatamente esse campo, o convidado
recebe "sucesso" onde não há implementação — violando a regra do projeto
("nunca alegue suportado sem teste de integração e registro na matriz").

### 4.1 Dados falsificados retornados com sucesso

| API | Local | Dado falso |
|---|---|---|
| `gethostbyname` / `getservbyname` / `WSAAsyncGetHostByName` | `ws2_32.cpp:1549-1572, 1675-1684` | Sempre `0.0.0.0`/"localhost"/porta 22; `WSAAsyncGetHostByName` retorna ponteiro de `hostent` usado como task handle e buffer do chamador nunca preenchido |
| `Process32First` / `Process32Next` | `toolhelp.cpp:204-249` | Processo fixo `"process.exe"` (PID 1000, 4 threads) com sucesso; depois "no more files" |

> **Errata (2026-10-18, varredura de correção):** `Process32First`/`Next`
> (A e W) são **implementação real** em `toolhelp.cpp` — `CreateToolhelp32Snapshot`
> enumera `/proc`, `Next` avança `next_index` e o fim devolve
> `ERROR_NO_MORE_FILES`, com fixture própria (`tl_toolhelp.exe`). O achado
> acima está desatualizado e foi revertido durante o sweep; As exports
> permanecem `ExportSupport::Full`.
| `InsertMenuA/W`, `ModifyMenuA/W` | `menu.cpp:788-841` | Descartam todos os argumentos e retornam `TRUE` (menus ficam vazios) |
| `wsprintfW` | `misc.cpp:1075-1090` | Ignora `va_list`; retorna o literal do formato |

### 4.2 No-op com falso sucesso (estrutura quebrada invisível)

- **`CREATE_SUSPENDED` ignorado** na criação de thread + **`ResumeThread`**
  retorna 0 falso (`thread.cpp:620-624`).
- **`WaitForThreadpoolWorkCallbacks`** no-op (corrida com UAF do worker
  `detach()`; `g_tp_works` sem mutex — `thread.cpp:105-112, 505-508`).
- **GDI:** 104 `set_last_error(kErrorSuccess)` em `gdi32.cpp` (81 com
  `(void)hdc`) — desenho (BitBlt, ExtTextOut, PatBlt etc.) não desenha nada e
  finge sucesso.
- `TerminateThread` no-op; `DeviceIoControl` sucesso para qualquer IOCTL;
  `UnregisterClassW`; `SendMessageTimeout*`; `EnumWindows`/hooks
  (`SetWinEventHook`/`UnhookWinEvent`), `CreateIcon`, `SetMenuInfo`,
  `DialogBoxIndirectParamW` (retorna `IDOK` fixo), `ioctlsocket` fora de
  `FIONBIO`, `WSAIoctl` (bytes_returned=0), `OpenEventW`/`OpenFileMappingW`/
  `OpenSemaphoreW` fakes, `LockFile/UnlockFile` stub.

### 4.3 Violação da regra de runtime genérico

- **`src/runtime/gui_controls.cpp:1110-1196`** — strings de um app demo
  hard-coded no runtime: `"My tasks"`, `"New task"`, `"Search todos..."`,
  `"Delete"` (cor vermelha para qualquer app com botão Delete), colunas fixas
  `"ID/Title/Description/Priority/Status/Created"` em vez das colunas
  reais, badge `#id` na toolbar. `LVM_INSERTCOLUMN` é stub (`:710-713`,
  `return wparam`). Código exclusivo de aplicativo deve viver em
  `compat/apps/<app-id>/`, não em `src/runtime/`.

## 5. Falhas funcionais (apps quebrarão sem crash óbvio)

1. **`PostQuitMessage` invisível para `GetMessageA` bloqueado** —
   `message.cpp:883-898` checa `g_quit_requested` só na entrada; o loop idle
   (`:937-1144`) bloqueia sem re-checar → pump morto. O padrão correto já
   existe em `MsgWaitForMultipleObjectsEx` (`:1860`) — replicar.
2. **`CreateProcess` ignora `lpEnvironment`** (`process.cpp:309`) e o filho
   herda `::environ` do **host** cru (`:464`): `SetEnvironmentVariableA` não
   chega ao filho; variáveis internas `TL_*` vazam para o convidado.
   Fix: reconstruir o ambiente do filho a partir do map do convidado.
3. **DLLs carregadas via `LoadLibraryA/W` ficam fora do view de unwind** —
   `module_graph.cpp:1042-1078` vs `seh/unwind.cpp:303-311` (só cobre a imagem
   principal): exception desempilhando por frame dessas DLLs avança como leaf
   sem restaurar registradores e **continua a busca em contexto corrompido**
   (aceita resultado falso — pior que falhar).
4. **Pool de 16 fontes em `CreateFontA` nunca liberada** (`gdi32.cpp:204-234`)
   — 17ª chamada → `ERROR_NOT_ENOUGH_MEMORY`.
5. **Tolerância UPX/R29 descarta dados silenciosamente** (`pe_reader.cpp:426-445`,
   4 usos): diretórios exports/TLS/exceptions/relocs em seção virtual tornam-se
   "inexistentes" sem trace — mascara PE malformado; para relocs, o
   `PeInfo.relocation_directory_size` permanece ≠ 0 e o mapper rejeita a imagem
   com mensagem enganosa ("relocação corrompida") em vez de "diretório não
   populado". Divergência parser↔mapper confirmada por leitura.
6. **Recursos truncados em `--report`** — `resource_inspector.cpp:417-434`
   devolve span truncado silenciosamente; manifest/version parseados sobre
   blob parcial sem indicador.

## 6. Robustez, DoS e corridas

### 6.1 DoS

- **Traversal combinatório da árvore de recursos** —
  `resource_inspector.cpp:391-405` e `546-571`: limites por diretório (1024
  entradas) e por profundidade (3), mas **sem orçamento global nem
  visited-set**; offsets são absolutos em `rsrc`, então loop próprio é
  trivial. Com ~8 KiB de tabela: ≥ 10¹² visitas. Acionável via
  `tradutorlinux --report` (`report.cpp:860-861` chama incondicionalmente) e
  via convidado em `GetFileVersionInfoW` (`version.cpp:85-90`).
- **`CreateProcess` lê arquivo sem cap** — `process.cpp:69-90`: lê até EOF
  sem limite (os demais leitores impõem 512 MB). `std::bad_alloc` em função
  `noexcept` → `std::terminate`/OOM-killer.
- **Rust: `profile_parser.rs:1052-1087`** — `string_offset` reescaneia toda a
  tabela de strings para cada referência (O(N²)); dentro dos limites legais
  (262k strings/1 MiB) pode custo ~17 bilhões de iterações de CPU.

### 6.2 Corridas / integridade

- **ws2_32:** `find_socket` sem `g_sockets_mutex` na maioria das chamadas →
  `closesocket` de outra thread troca o fd por baixo (∗slot = {} + close);
  `recv/send/accept` bloqueantes podem operar **outra conexão** reutilizada
  pelo SO (`ws2_32.cpp:226-232` e chamadas).
- **`errno_to_wsa`** default `WSAENOTSOCK` (10038) para qualquer errno não
  mapeado + uso de literal `ENOBUFS (105)` como código WSA
  (`ws2_32.cpp:329-352, 468, 548, 838, 1398`) — lógica de retry do convidado
  errada.
- **`g_map_regions` lido fora do lock** — `memory_validator.cpp:75-87`:
  `ensure_cache_valid` testa `empty()` sem lock enquanto outra thread pode
  estar em `rebuild_cache_locked` (UB formal com runtime multithread real).
- **`g_windows` mutado sem `g_gui_state_mutex`** — `winapi.cpp:805-846`
  (`create_logical_control`; parcialmente mitigado pelo gate de thread GUI,
  não aplicado nos call sites de comctl32).
- **`g_find_slots` (16 slots) sem mutex** — `file_find.cpp:70-170` inteiro;
  `FindClose` de uma thread limpa slot usado por outra (UAF/dup-free).
- **`g_tp_works`/`g_tp_timers` sem mutex** — ver A3/4.2.
- **Escrita global TLS:** `g_guest_tls_slots[i]=value` (`thread.cpp:361`) —
  array compartilhado por todas as threads.
  > **Errata (2026-10-18, varredura de correção):** falso positvo —
  > `g_guest_tls_slots` é `thread_local` (`runtime_thread_state.hpp:50`,
  > definido em `winapi.cpp:92`): cada thread hospedeira tem seu array.
  > Nenhuma escrita global de TLS existe; sem correção necessária.
- **`wininet`: `g_internet_mutex` travado durante `easy_perform`** (até 60 s)
  — serializa toda a API para o processo (`wininet.cpp:861-897`; projeção,
  não crash).

### 6.3 Acesso direto a memória do convidado (fora das primitivas)

- **`IdnToAscii`/`IdnToUnicode`** (`winapi.cpp:1787-1812`, registro Full):
  dereferência direta de `src` hostil e escrita direta em `dst` → SIGSEGV no
  runtime em vez de `ERROR_INVALID_PARAMETER` controlado.
- **`BCryptGenRandom`** (`winapi.cpp:1843-1866`): escreve no buffer do
  convidado por `getrandom`/`read` diretos, fora de `write_guest_memory`.
- **Relocations podem patchear headers/padding** — `image_mapper.cpp:256-271`
  valida alvo só contra o mapping inteiro (headers/padding incluídos);
  higiene, não exploração (reloc roda antes do mprotect).
- **Páginas órfãs do mapping ficam RW** — `image_mapper.cpp:422-441`: páginas
  sem interseção com região nenhuma mantêm PROT_READ|WRITE em vez de
  `PROT_NONE`.

### 6.4 ABI / SEH

- **`RtlCaptureContext` (Full) clobbera registradores não-voláteis Win64** —
  `unwind_capture.S:49-56`: `leaq .Lname(%rip), %rdi` destrói RDI
  determinísticamente; a `callq trace_assembly_function_entry` SysV pode
  destruir RSI via callee. Viola o contrato de
  `docs/arquitetura/abi-x64.md:33-40`. Fix: push/pop RDI em torno do bloco de
  trace e usar registrador volátil nos dois ABIs (R10/R11).
- **`cxx_eh.S` chama SysV com pilha desalinhada** — `cxx_eh.S:16-19, 33-36`:
  `andq $-16` + `subq $8` deixa a segunda `callq` com RSP ≠ 0 mod 16; a
  primeira `callq` depende da paridade deixada pelo funclet
  (`cxx_eh.cpp:1787-1797` mostra que é não-determinística). Crashes `movaps`
  latentes.
- **`validate_guest_stack_range` ignora o parâmetro `writable`**
  (`unwind.cpp:426-449`) — writes via fallback `/proc/self/mem` podem COW em
  páginas R-only (desvio do modelo Win32; sem escalada).
- **`guest_executable_address` reabre `/proc/self/maps` por chamada**
  (`winapi.cpp:159-179`) — custo/perf; inconsistência com o cache do
  validador.
- **Edge:** `entry_point == 0` passa no gate de página executável
  (`loader/process.cpp:47-53`) — salto para os bytes `MZ` (SIGILL); rejeitar
  entry 0 para EXE.

### 6.5 Menções honoráveis (baixa/média, registrar na matriz)

- `ioctlsocket` só `FIONBIO`; `FIONREAD` falso 0.
- `FindResourceA` reinterpreta `char*`↔`uint16*` (`resource.cpp:294-299`).
- `SubtractRect` bugado (`misc.cpp:1132-1147`).
- `::open` sem `O_CLOEXEC` (`file_io.cpp:236, 298`); share_mode ignorado;
  `DeviceIoControl` sucesso universal.
- `GetWindowThreadProcessId` sempre pid/thread atuais (`window.cpp:1304`).
- `g_wait_address_versions` cresce sem teto (`sync.cpp:627`).
- `UnregisterClassW` no-op TRUE; `DialogBoxIndirectParamW` retorna `IDOK`.

## 7. Build, CI e documentação

| # | Severidade | Achado | Local |
|---|---|---|---|
| B1 | ✅ verificado — alto | **CI: lista do clang-tidy aponta arquivos deletados** — `src/cli.cpp` e `src/runtime/winapi.cpp` não existem mais (movidos para `src/cli/` e `src/runtime/core/`); cobertura atual narrou a ~8 TU de 100+; risco de falha por arquivo inexistente | `.github/workflows/ci.yml:41-46` |
| B2 | alto | **DEB com dependência formal quebrada** — falta `zlib1g` (e `libstdc++6`, `libgcc-s1`); `objdump -p build/debug/src/tradutorlinux` mostra `NEEDED libz.so.1` etc. | `CMakeLists.txt:61` vs `:11` e `src/CMakeLists.txt:178` |
| B3 | alto | **README quickstart omite dependências REQUIRED** — `libx11-dev`, `libwayland-dev`, `libwayland-bin`, `libxkbcommon-dev`, `wayland-protocols` (xdg-shell.xml), `libxtst-dev` | `README.md:48` vs `src/CMakeLists.txt:174-187`, `tests/CMakeLists.txt:588` |
| B4 | médio-alto | **Qt6 `REQUIRED` incondicional** — impossível build headless (CLI/.Left unitários) sem Qt; falta opção `TL_ENABLE_GUI=OFF` | `CMakeLists.txt:10` |
| B5 | médio | **Preset `tsan` existe, nunca roda no CI** — única cobertura de concorrência por sanitizador ausente do CI | `CMakePresets.json:66-72`; `ci.yml` |
| B6 | médio | **Docs desatualizados** — PROJETO.md §7 mostra árvore pequena (`src/{cli,main,diagnostics}`) que não existe mais; PROJETO.md §3.3/§10 narram o MVP como vigente; README "Fase 13/13.14" está atrás do ROADMAP (R25–R29 concluídos) | `PROJETO.md:134-168, 72-80, 190-196`; `README.md:9` |
| B7 | médio | **`docs/compatibilidade*.md` divergentes entre si pós-R29** — Rufus/HWiNFO64 ainda "malformed/not-attempted" nos docs vs ROADMAP R29 (`--report` exit 0) e testes esperando 0; contagens conflitantes E31 ("24/3" vs "25/2"); "668 testes" vs 1.016 reais; APIs citadas sem rastro de teste (`DLGTEMPLATEEX`, `TerminateThread` 0 hits em tests/) | `compatibilidade-aplicativos.md:191-192, 289, 263-265, 105-109`; `compatibilidade.md:55-57` |
| B8 | médio | **AGENTS.md não menciona subsistema Rust opt-in** (`TL_BUILD_RUST`, `Cargo.toml`, `src/rust/`) — diretriz de linguagens desatualizada | `AGENTS.md:69-75` |
| B9 | médio-baixo | **Corpus `Aplicativos_Windows_Populares` é local-only, sem SHA-256 pinado** — claims de smoke 7zFM/putty/WinRAR/Notepad++ não reexecutáveis fora da máquina do dono; degradação de teste está correta (não-registro quando ausente; skip 77) | `tests/CMakeLists.txt:9-91`; `tests/apps/README.md` |
| B10 | baixo | README.md linka `CONTRIBUTING.md`/`CODE_OF_CONDUCT.md`/`SECURITY.md` inexistentes | `README.md:96-98` |
| B11 | baixo | 5 checagens tautológicas (`size > uint64::max` em 64-bit) nos adapters — dead code com falsa confiança de defesa | `rust_path_validator.cpp:89`; `rust_profile_parser.cpp:665`; `rust_msix_parser.cpp:485`; `rust_pe_parser.cpp:888`; `rust_app_catalog_parser.cpp:654` |
| B12 | baixo | Limites duplicados header↔Rust sem teste de paridade (ex.: `TL_PE_LIMIT_MAX_STRING_BYTES` 65534 ↔ `MAX_CSTRING` 65535 — consistentes hoje, sem teste que proteja) | `rust_pe_parser.h:153` ↔ `pe_parser.rs:57` |

## 8. FFI Rust ↔ C++ (expande a seção 6/B11)

### 8.1 Verified OK (sem risco)

- As assinaturas dos **14 exports casam com os headers** (`u64`, `char*`,
  `#[repr(C)]` 24/48 bytes); todos têm `catch_unwind`; contrato zero-copy
  caller-owned (sem risco de mismatch de alocador); parsers stateless e
  thread-safe; zero crates externas; toolchain pinada 1.97.1 e
  `cargo --locked --offline`; **CI tem job rust-ffi dedicado** (o rumor "Rust
  nunca construído/testado" é falso).

### 8.2 Achados

| Sev. | Achado | Local |
|---|---|---|
| médio | O(N²) `string_offset`/`put_ref` (ver 6.1) | `profile_parser.rs:1052-1087` |
| médio | `ffi_entry` do profile: `output_required` não é reportado quando `error==null`, e o parse inteiro roda antes do erro; nullabilidade diverge das 4 famílias (catalog/pe/msix têm contratos diferentes) | `profile_parser.rs:1395-1411, 1342` |
| médio-baixo | Único parser sem guard `input.is_null()` antes de `from_raw_parts` (UB em chamador externo futuro; hoje os adapters nunca passam null+len>0) | `msix_parser.rs:2145-2157` |
| baixo-médio | Adapter de catalog é o único sem try/catch próprio (o try/catch mora no chamador `app_catalog.cpp:443-458`); novos chamadores herdam exceções atravessando a fronteira | `src/catalog/rust_app_catalog_parser.cpp:628+` |
| baixo | 5 checagens tautológicas (ver B11) | adapters |
| baixo | Limites sem teste de paridade (ver B12) | header ↔ `.rs` |

## 9. Verificado correto (anti-falso-positivo)

1. **Pipeline PE:** `ByteReader.has_range` sem wrap (`pe_reader.cpp:102-104`);
   `rva_to_file_offset` com clamps u64 duplos; exports/ordinais com validação
   completa (name_count ≤ function_count, overflow, duplicata, forwarder);
   `.pdata` com begin<end ≤ SizeOfImage, ordenação, CHAININFO com detecção de
   ciclo; `memcpy/memset` do mapper chumbados por `SizeOfImage`.
2. **W^X:** `section_permissions` nunca devolve RWX; união degrada X+W → RW;
   headers por último; `write_image_bytes` re-protege (degrada para PROT_READ
   se restauração falha); TLS callbacks validados via permissão real — nada
   fica RWX/W+X ativo.
3. **Primitivas de memória convidada** (`memory_validator.cpp:154-217`):
   `process_vm_readv/writev` + fallback `/proc/thread-self/mem` com
   invalidação de snapshot; consumidores testam `Success` explicitamente;
   `copy_guest_cstring` trata `bad_alloc`.
4. **Unwinding FH4 (R26):** limite de 16 cleanups com `visited[]` por estado,
   rejeição de ciclo, alinhamento revalidado e degradação controlada
   (bate com `unwinding-x64.md:96-101`).
5. **Ciclo de vida FLS/TLS/TEB:** mutexes corretos, callbacks executados fora
   do lock, TEB mmap/munmap por thread, GS usado (glibc TLS intacta).
6. **Forwarders A→B→A:** depth 32 + visited-set (sem recursão infinita);
   case-insensitivity consistente em toda a cadeia.
7. **Wiring de testes:** 33 test_*.cpp todos ligados; 1.016 testes fecham;
   discovery por gtest; skip codes corretos para corpus/display/Proton.
8. **Isolamento de extensão:** `compat/apps/7zip` em alvo separado, sem
   vazamento no core (meta do AGENTS.md cumprida).
9. **ws2_32 staging** de send/recv com `read/write_guest_memory`, clamp de
   length e flags honestos (`ws2_32.cpp:599-727`).
10. **wininet:** cascade-close de descendentes, CA file obrigatório, timeouts
    herdados, bloqueio de headers sensíveis (`wininet.cpp:1093-1108, 511-624`).
11. **CreateDIBSection** com matemática de overflow explícita e cap
    (`gdi32.cpp:487-569`).
12. **Rust FFI:** ver seção 8.1.

## 10. Ordem de ataque recomendada

1. **A1** (HeapFree) — ~5 linhas + fixture de canário corrompido.
2. **A3** (SubmitThreadpoolWork) — reusar path de `CreateThread`; enquanto não
   houver fixture, rebaixar para `Stub` com trace.
3. **A2** (ExitProcess cross-thread) — flag + despertar main.
4. **Quick wins de CI/build** (sem risco): B1 (lista clang-tidy), B2 (deps
   DEB), B10 (links 404).
5. **Sweep de honestidade da matriz (seção 4):** reclassificar `Full` →
   `Stub/NotSupported` nos no-ops listados (grep + registro em
   `docs/compatibilidade.md`), depois implementar os que tiverem
   aplicativo-alvo; mover regras de app de `gui_controls.cpp` para
   `compat/apps/`.
6. **DoS:** orçamento de visitas no resource inspector; cap 512 MB no leitor
   do CreateProcess; O(1) no `string_offset` do profile.
7. **Corridas** (seção 6.2): `memory_validator` cache, `g_find_slots`,
   `g_tp_works`, fd↔slot do ws2_32.
8. **ABI:** push/pop RDI em `unwind_capture.S`; alinhamento em `cxx_eh.S`.
9. **Docs:** B6/B7/B8 (tree do PROJETO.md, contagens da matriz, AGENTS.md +
   Rust).
