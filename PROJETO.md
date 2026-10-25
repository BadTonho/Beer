# Beer — Runtime de Compatibilidade Win32 para Linux

## 1. Visão geral

O Beer é um runtime de compatibilidade **sério e de produção**: um runtime capaz de executar, de forma progressiva e confiável, **classes cada vez mais amplas de aplicativos Win32 reais** no Linux, sem máquina virtual e sem emular a CPU — com foco em uso diário, não apenas educacional.

O objetivo estratégico é tornar útil no desktop Linux um conjunto crescente de aplicativos reais do corpus `Aplicativos_Windows_Populares/`, completando fluxos verificáveis de instalação, abertura, interação e gravação. `--report`, `--trace`, `exit code` e resultados observáveis sustentam cada declaração de compatibilidade. Cada capacidade precisa ser implementada, testada e publicada antes de ser considerada suportada.

O alvo inicial é deliberadamente restrito:

| Dimensão | Suporte inicial |
|---|---|
| Arquitetura do programa | PE32+ x86-64 |
| Máquina hospedeira | Linux x86-64 |
| Tipo de aplicativo | Console Win32 e GUI Win32 mínima experimental |
| Dependências | Conjunto documentado de APIs `KERNEL32`, `USER32` e `GDI32` |
| Fora do escopo inicial | PE32 x86, ARM, .NET, COM amplo/ActiveX, drivers, GUI Win32 ampla, GDI completo, DirectX e anticheat |

Com CPU igual nos dois lados, as instruções x86-64 do programa podem ser executadas nativamente. O trabalho do Beer é carregar a imagem PE, preparar o contexto de processo esperado por ela e fornecer as APIs Windows que o programa importa.

### Linguagem principal

O projeto será escrito principalmente em **C++20**. C poderá ser usado em estruturas compatíveis com o formato PE, interfaces C e trechos em que uma ABI simples seja importante. Assembly x86-64 ficará restrito às fronteiras realmente necessárias, como trampolins ou bootstrap do processo.

Essa divisão mantém o controle de baixo nível sem transformar todo o projeto em assembly ou em C puro. A regra é deixar a fronteira entre o código Windows e o código Linux pequena, explícita e coberta por testes.

## 2. O problema, com precisão

Não é correto resumir o funcionamento a “traduzir syscalls do Windows para syscalls do Linux”. A maior parte dos aplicativos chama funções de DLLs Windows; essas funções, por sua vez, dependem de convenções de chamada, estruturas de processo, handles, erros e semânticas próprias do Windows.

Portanto, o projeto implementa uma **camada de compatibilidade em espaço de usuário**. Ela encaminha operações ao Linux quando isso preserva o comportamento definido para o escopo suportado.

Exemplo simplificado:

```text
Programa PE32+ x86-64
       │ chamadas importadas (Win32)
       ▼
PE loader + resolvedor de imports
       │
       ▼
Runtime de compatibilidade
  ├─ ponte de ABI Microsoft x64 ↔ System V AMD64
  ├─ APIs Win32 suportadas
  ├─ handles, memória, erros e caminhos
  └─ rastreamento e diagnóstico
       │
       ▼
Linux / POSIX
```

## 3. Objetivos

### 3.1. Objetivos de produto

- Manter os alvos já suportados e priorizar fluxos de desktop Linux completos
  para aplicativos PE32+ x86-64 já presentes no corpus.
- Buscar cobertura prática de classes comuns de aplicativos Win32 PE32+ x86-64
  de espaço de usuário, sem declarar que todo `.exe` dessas classes funciona
  antes de haver evidência reproduzível.
- Integrar instalação, biblioteca, execução, arquivos e remoção ao desktop Linux
  sem perder o isolamento de prefixos e os diagnósticos do runtime.
- Informar com clareza quando um executável ou API ainda não é suportado.
- Produzir rastros de execução que expliquem imports, chamadas e falhas.
- Manter uma matriz de compatibilidade e testes automatizados para cada função implementada.

### 3.2. Objetivos de produto sério

- Entregar valor para fluxos representativos do portfólio real, medindo separadamente resolução de imports, execução, resultado observável e limitações via `--trace` em `docs/diagnostico.md`.
- Garantir estabilidade: loader, ABI, memória, imports, TLS, unwind/SEH e diagnósticos com `exit code` `71/72` e `fault-address` antes de ampliar famílias de DLL.
- Manter qualidade de produção: warnings rigorosos, `CTest` `sanitize`, `LSAN_OPTIONS`, `Xvfb` e matriz `docs/compatibilidade.md` atualizada a cada API.

### 3.3. Primeiro MVP (concluído — histórico)

O primeiro marco funcional foi executar `tl_hello.exe`, um binário de teste
próprio para x86-64 que importava apenas:

- `GetStdHandle`
- `WriteFile`
- `ExitProcess`

O marco está fechado e permanece protegido em regressão (suite unitária e
trace reproduzível). A fila vigente está no [`ROADMAP.md`](ROADMAP.md); o
histórico de fases fica em `feitos/`. Se uma importação desconhecida
aparecer, o runtime continua encerrando de forma controlada e explicando
qual módulo, símbolo e mecanismo não foram suportados.

### 3.4. Fora de escopo no estágio atual

- Compatibilidade presumida com qualquer executável sem análise, implementação e regressão correspondentes.
- Interface gráfica Win32 além do subconjunto experimental, GDI completo, DirectX, áudio e GPU.
- COM/ActiveX amplo, automação `IDispatch`, .NET, drivers e serviços Windows. Os
  fixtures de `ole32.dll` e streams em memória documentados na matriz são
  contratos mínimos isolados, não suporte geral a COM.
- Suporte a 32 bits, ARM, WOW64 ou execução cruzada de arquitetura.
- Segurança de executáveis não confiáveis. Compatibilidade não é sandbox: um `.exe` executado nativamente tem os privilégios do usuário atual.

Esses limites descrevem o estágio atual, não o limite definitivo do produto. A expansão para novas classes exige uma fase ou aplicativo-alvo explícito, contratos técnicos, testes de integração e atualização da matriz de compatibilidade.

O histórico das etapas anteriores está em [`feitos/`](feitos/). O plano vigente
de experiência desktop está no [ROADMAP.md](ROADMAP.md).

## 4. Arquitetura proposta

| Componente | Responsabilidade |
|---|---|
| **CLI / Runner** | Receber o caminho do `.exe`, configurar logs e apresentar erros. |
| **Leitor de PE** | Validar headers e diretórios; expor seções, imports e relocations sem executar código. |
| **Image mapper** | Reservar memória, copiar seções, aplicar proteções e relocations. |
| **Loader de módulos** | Registrar módulos compatíveis e resolver imports normais, por ordinal e encaminhamentos suportados. |
| **Pontes de ABI** | Permitir que código Microsoft x64 chame uma API hospedada no Linux e retorne corretamente. |
| **Runtime Win32** | Implementar APIs compatíveis, handles, `GetLastError`, memória e caminhos. |
| **Diagnóstico** | Produzir `--trace`, relatório de imports e mensagens de incompatibilidade acionáveis. |
| **Testes** | Exercitar parser, loader, APIs e binários de integração determinísticos. |

As “DLLs” iniciais não serão arquivos falsos soltos. Serão módulos registrados pelo loader, cujas exportações apontam para funções de compatibilidade e trampolins de ABI. Essa escolha reduz o número de partes móveis do MVP e preserva o modelo que um executável PE espera encontrar.

## 5. Regras de projeto

- Implementar comportamento documentado e testado, não apenas nomes de funções.
- Acrescentar uma API somente quando existir um programa de integração que a justifique.
- Nunca chamar o ponto de entrada de uma imagem antes de mapear seções, aplicar relocations, resolver imports e preparar o contexto mínimo necessário.
- Implementar páginas de memória com permissões coerentes; não manter toda a imagem RWX por conveniência.
- Tratar executáveis de entrada como dados hostis durante o parsing. Parser, tabelas e limites devem ser validados e testados contra entradas malformadas.
- Usar documentação pública e testes de comportamento como referência. Qualquer reaproveitamento de código de terceiros exige revisão da licença aplicável antes de distribuição.

## 6. Estratégia de testes e qualidade

| Nível | O que verificar |
|---|---|
| Unitário | Leitura de estruturas PE, validação de limites, conversões e APIs isoladas. |
| Integração | Executáveis de teste construídos no repositório com imports conhecidos. |
| Regressão | Saída, código de retorno, trace e erros esperados de cada aplicação-alvo. |
| Robustez | Arquivos PE truncados, offsets inválidos, imports malformados e imagens incompatíveis. |
| Manual | Depuração com `gdb`, inspeção de imagem com `llvm-objdump` e rastreamento do runtime. |

Cada marco deve ser reproduzível sem Windows. `mingw-w64` pode gerar os binários de teste, mas os primeiros exemplos devem controlar cuidadosamente CRT e imports para que cada dependência nova seja intencional.

## 7. Estrutura de pastas

```text
TradutorLinux/
├── PROJETO.md, README.md, ROADMAP.md
├── Cargo.toml                      # crate Rust opt-in (TL_BUILD_RUST)
├── CMakeLists.txt
├── CMakePresets.json
├── AGENTS.md
├── docs/                           # contratos, auditoria e matrizes
│   ├── arquitetura/                # ABI, SEH/x64, aplicativos-bloqueios
│   ├── compatibilidade.md, compatibilidade-runtime.md
│   ├── compatibilidade-aplicativos.md, compatibilidade-backends.md
│   ├── diagnostico.md, catalog.md, requisitos-aplicativos.md
│   └── (formatos/, api/ nas fases em que os contratos demandarem)
├── src/
│   ├── backend/                    # proton/proton_runtime (stubs de integração)
│   ├── catalog/                    # parsers de catálogo (TLAC) e FFI Rust
│   ├── cli/                        # runner, opções, relatórios
│   ├── compat/                     # validador e parser de perfil Rust FFI
│   ├── diagnostics/
│   ├── gui/                        # X11 e estados de GUI
│   ├── loader/                     # grafo de módulos, image_mapper, import_resolver
│   ├── main.cpp
│   ├── package/                    # msix/appx (FFI Rust)
│   ├── pe/                         # leitor PE e parser diferencial Rust
│   ├── prefix/
│   ├── process/
│   ├── runtime/                    # dlls/, core/, seh/
│   ├── rust/                       # código-fonte do crate (pe/msix/profile parsers)
│   └── util/
├── tests/
│   ├── apps/<app-id>/              # smokes e fixtures isolados por aplicativo
│   ├── samples/
│   ├── test_*.cpp
│   └── pe_builder.hpp, test_support.hpp, CMakeLists.txt
├── compat/
│   └── apps/<app-id>/              # extensões isoladas, com seleção explícita
└── feitos/                         # roadmaps R1–R29 e marcos fundacionais (histórico)
```

Extensões específicas de aplicativos ficam separadas em `compat/apps/<app-id>/`,
em alvos próprios, sem dependência do `tradutorlinux_core` sobre elas. A extensão
host-side do 7-Zip já integra essa base e só é ativada por perfil schema 4 com
`"extension": "7zip"`, conforme o contrato em
[`docs/arquitetura/perfis-compatibilidade.md`](docs/arquitetura/perfis-compatibilidade.md).
Ela não fornece DLL. Novas extensões ou ampliações de contrato exigem etapa
explícita do roadmap, aprovação correspondente, testes e registro na matriz;
criar um diretório não autoriza gerar ou baixar DLLs. O crate Rust (`src/rust/`,
desde `TL_BUILD_RUST=ON`) é opt-in e acompanha os últimos contratos de parser
registrados em `docs/compatibilidade-backends.md`.

## 8. Stack inicial

- **Linguagem:** C++20, com C ou assembly somente nas fronteiras em que forem necessários.
- **Build:** CMake + Ninja.
- **Compilador:** Clang ou GCC no Linux, com sanitizers em testes quando possível.
- **Binários de teste:** `mingw-w64` para x86-64 e assembly/linking explícito quando for preciso eliminar CRT.
- **Inspeção e depuração:** `llvm-objdump`, `readelf`, `gdb`, `strace` e `ltrace`.
- **CI:** build, testes unitários, testes de integração e análise estática desde a primeira fase.

## 9. Riscos e respostas

| Risco | Resposta |
|---|---|
| Escopo explode com APIs | Compatibilidade publicada e aplicações-alvo antes de implementar novas funções. |
| ABI e pilha incorretas | Pontes isoladas, testes de chamada e documentação por fronteira. |
| PE malformado causa falha | Validação rigorosa de offsets/tamanhos, fuzzing e sanitizers. |
| Binário de teste traz dependências ocultas | Fixtures sem CRT e relatório automático de imports. |
| Semântica difere do Linux | Contratos explícitos, `GetLastError`, testes de regressão e limitações declaradas. |
| Execução de software malicioso | Não prometer isolamento; considerar sandbox Linux como recurso futuro independente. |

## 10. Critérios de sucesso

- **Marco 1 (concluído):** o leitor identifica corretamente um PE32+ válido e rejeita entradas inválidas com segurança.
- **Marco 2 (concluído):** uma imagem mínima é mapeada, relocada e tem seus imports resolvidos sem executar código fora do escopo.
- **MVP (concluído):** `./beer tests/samples/tl_hello.exe` escreve a saída correta e encerra com o código esperado.
- **Qualidade do MVP (concluído):** todas as APIs usadas pelo exemplo possuem testes; imports ausentes são diagnosticados; o trace é reproduzível.
- **Experiência desktop (vigente):** completar e comparar fluxos cotidianos de aplicativos PE32+ x86-64, integrar biblioteca e arquivos ao desktop Linux e publicar limitações por backend na matriz de compatibilidade.

## 11. Exemplo de uso futuro

```bash
./beer --trace tests/samples/tl_hello.exe
```

Saída esperada:

```text
[loader] PE32+ x86-64 reconhecido
[imports] kernel32.dll!GetStdHandle -> suportado
[imports] kernel32.dll!WriteFile -> suportado
Olá do Windows no Linux!
[process] ExitProcess(0)
```
