# Matriz de compatibilidade

Este arquivo é o índice da matriz de compatibilidade. Os contratos foram separados por tema para facilitar manutenção; a matriz continua registrando resolução de imports, execução, nível funcional e limitações. Ela não é uma promessa de compatibilidade geral com Windows.


Esta matriz registra resolução de imports, resultados de execução e limitações
conhecidas; ela não é uma promessa de compatibilidade geral com Windows.

## Como ler a matriz

Há três dimensões independentes:

- **`--report`/imports:** `supported` significa que todos os imports estáticos
  foram resolvidos; `unsupported` significa que ao menos uma dependência ou
  mecanismo não foi resolvido. Isso não comprova execução.
- **execução:** `not-attempted`, `passed` ou uma falha controlada como
  `GuestTimeout 72`, `GuestFault 71`, `GuestResourceLimit 73` ou um código
  explícito do convidado.
- **nível funcional:** o catálogo usa `inicia`, `fluxo principal restrito`,
  `fluxo principal` e `uso diário` somente quando há fluxo representativo,
  resultado observável e limitações publicadas.

Assim, um aplicativo pode ter `supported` na resolução de imports e continuar
`execution-failed` ou sem nível funcional no catálogo.

O probe local do PuTTY mantém essa distinção: as etapas E39–E45 validam o
transporte inicial de loopback, as notificações `WSAAsyncSelect` e `FD_CLOSE`,
`SSH_MSG_KEXINIT` observado nos dois sentidos, seleção nominal de algoritmos e a sequência controlada
`SSH_MSG_IGNORE` + `SSH_MSG_DISCONNECT`, mas o fluxo ainda termina em
`GuestTimeout 72`; isso não promove o aplicativo nem o protocolo SSH a suporte
funcional.

### Análise aprofundada de requisitos com `--report`

O comando `beer --report <app.exe>` realiza inspeção estática aprofundada sem executar a aplicação, antecipando requisitos de infraestrutura e limitações conhecidas:
- **Aceleração 3D (DirectX/Vulkan):** detecta dependências de `d3d11.dll`, `d3d12.dll`, `dxgi.dll`, `d3d9.dll`, `vulkan-1.dll` e `xinput1_4.dll`, recomendando `backend.kind: proton` quando gráficos 3D forem essenciais.
- **Ambiente gerenciado (.NET/CLR):** identifica binários dependentes de runtime .NET (`mscoree.dll` ou diretório COM descriptor), orientando a execução via `dotnet` ou Proton na ausência de compilação nativa AOT.
- **Drivers de kernel e anticheat:** detecta módulos de anticheat (EasyAntiCheat, BattlEye, Riot Vanguard, PunkBuster, Denuvo) e chamadas de gerenciamento de serviços/drivers (`CreateServiceW`, `OpenSCManagerW` em `advapi32.dll`), sinalizando a impossibilidade de execução nativa de módulos ring-0.
- **Packers e proteções (UPX, Themida, VMProtect):** detecta código compactado e seções anômalas com permissões W+X ou sem dados físicos (`raw_size=0`), prevenindo violações da política W^X em runtime nativo.
- **Mitigações de segurança (`DllCharacteristics`):** inspeciona o suporte a ASLR, High-Entropy VA, DEP/NX, Control Flow Guard (CFG) e AppContainer.
- **Identidade e versão (`RT_VERSION`):** extrai nome do produto, versão e fabricante diretamente da árvore de recursos `.rsrc`.
- **Inventário de recursos e manifesto (`RT_MANIFEST`):** inventaria tipos de recursos embutidos e requisitos de UAC (`asInvoker`, `requireAdministrator`), DPI e versões de SO suportadas.

O teste CTest `popular_apps_report_matrix`, habilitado quando
`TL_POPULAR_APPS_DIR` aponta para o corpus pinado, repete a análise estrutural
dos 26 PE selecionados e do pacote MSIX. Ele verifica os códigos esperados de
sucesso (`0`), rejeição estrutural (`4`) e formato/arquitetura não suportados
(`5`) sem mapear ou executar os arquivos; a mesma matriz é executada nos
builds Rust ON e C++ OFF.

O teste `popular_apps_recursive_report_matrix` amplia essa verificação para
todos os 64 arquivos PE, DLL e MSIX encontrados recursivamente no corpus
pinado, incluindo os diretórios extraídos de 7-Zip e Notepad++. Ele executa
somente `--report`, sem iniciar DLLs, instaladores ou pacotes rejeitados. A
tolerância R29 ao Rufus e a resolução R47 de `NppConverter.dll`/`NppShell.dll`
moveram casos do estado de rejeição para exit 0, e a varredura de correções da
auditoria 2026-10-18 corrigiu expectativas obsoletas do script (achado B7). Em
18 de outubro de 2026, os dois builds retornam `38` casos com exit 0 (sucesso
de leitura/relatório), `26` com exit 5 (`Unsupported`) e nenhum com exit 4;
o script `verify_popular_apps_recursive_report.cmake` é a fonte oficial das
expectativas por arquivo.
Na rodada U5, ambas as matrizes passaram em Debug com diretórios temporários;
`--report` confirmou que não cria prefixo, mesmo se o caminho configurado
ainda não existir.

O teste `popular_apps_native_matrix` cobre quatorze casos de execução direta ou
rejeição pré-entry: os cinco binários 7-Zip (`7z_x64.exe`, `7zFM_x64.exe`,
`7-Zip/7z.exe`, `7-Zip/7zFM.exe`, `7-Zip/7zG.exe`) e os dois WinRAR terminam
com `0`, Rockstar preserva seu `ExitProcess(1)`, Rufus e HWiNFO64 são
rejeitados com `4` no parsing/mapeamento de seções (entry point fora de página
executável), o G HUB encerra com `0` e os dois GUP do Notepad++ são rejeitados
com `255` (`4294967295`). O G HUB recebe limite de 30 s de CPU/execução e 1 GiB
de memória porque o limite anterior de 3 s expirava antes do encerramento
observável. Cada caso recebe prefixo temporário e limites de recursos; isso é
evidência de comportamento controlado, não uma promoção geral de compatibilidade.

O teste `notepadpp_fh4_headless_smoke` acrescenta uma execução real do
`Notepad++/notepad++.exe` sem display: o runtime seleciona os catches FH4
tipados, executa a cadeia de cleanups do frame-alvo até o `target_state`
(`fh4-termination-cleanup`), não atinge `fh4-cleanup-limit`, não produz `guest-signal`
nem `guest-timeout` e termina com `ExitProcess(0)`. A fixture FH4 também rejeita um auto-link cíclico
com diagnóstico controlado. Isso valida apenas o caminho de inicialização e
C++ EH observado; o fluxo GUI continua dependente do smoke Xvfb e não é
promovido a suporte diário.

O teste `popular_apps_install_matrix` cobre doze casos controlados de instalação:
as autorizadas de Roblox (`72` após guest-timeout) e G HUB / alias (`6` após
`ExitProcess(0)`), o preparo controlado de HWiNFO64 (`prepare`, exit `4`) e as
rejeições pré-extração por arquitetura de máquina x86/PE32 de CPU-Z, GPU-Z,
HWMonitor, CapCut, Creative Cloud, Epic Installer, Office Deployment Tool e RTSS
(`5`, stage `parse`). Ele isola prefixo, `HOME`, configuração e `APPDATA`, exige
que não haja arquivos residuais, extração ou cadastro parcial nas rejeições.
G HUB e o alias recebem limite de 30 s; 3 s
encerravam o bootstrapper antes de `ExitProcess(0)` e impediam a avaliação do
estado `no-candidate`.

O teste `affinity_msix_install_smoke`, adicionado na rodada U5, analisa o MSIX,
instala e cadastra o pacote em prefixo limpo, confere o executável principal e
seu relatório .NET CLR, remove o cadastro e verifica que os arquivos persistem.
O executável gerenciado não é iniciado pelo runtime nativo.

O teste `popular_apps_x86_installer_rejection_matrix` verifica os quatro
containers de instalador do corpus (`7-Zip_x64_Installer.exe`,
`Notepad++_x64_Installer.exe`, `Everything_Search_x64.exe` e `RTSSSetup737.exe`).
Apesar dos nomes de payload x64, os arquivos são PE32 x86: `--report` e
`install` devem retornar `5` (`unsupported-architecture`), sem extração ou
cadastro. Os executáveis 7-Zip PE32+ existentes no corpus são cobertos por
smokes CLI e GUI separados. O corpus atual não inclui um executável standalone
do Everything para repetir seu fluxo `.efu`.

### Evidência de pacote Beer — Fedora 44 (24/10/2026)

O CPack gerou o RPM `beer-0.0.0-Linux.rpm`, instalado por DNF em um container
Fedora 44 descartável. O DNF resolveu as dependências ELF e
`desktop-file-utils`. Foram conferidos `rpm -q beer`, `beer --version`
(`Beer 0.0.0-dev`), `/usr/bin/beer-gui`, `beer.desktop`, `beer.png` e a
validação da entrada com `desktop-file-validate`. O CPack também gerou o DEB
com nome `beer`, arquitetura `amd64` e os mesmos binários, entrada de menu e
ícone. DEB continua sendo o gerador padrão.

Os testes focados de identidade passaram nos presets Debug e Sanitizers. No
Sanitizers passaram sete testes CLI (ajuda e versão, caminhos sem migração,
atalho/remoção, MIME e backend incompleto) e os smokes Qt de seleção, remoção
e associação, em modo offscreen. A execução Sanitizers usou
`ASAN_OPTIONS=detect_leaks=0`, pois
o LeakSanitizer falha ao enumerar testes neste ambiente. O CTest Qt completo
não pôde executar: as fixtures `tl_hello.exe` e de instalação não estão
geradas no preset atual. A checagem completa do smoke Qt depende dessas
fixtures.

Esta evidência de identidade não encerra U5. A validação Sanitizers do driver
C++ PuTTY e a validação Ubuntu permanecem pendentes.

### Evidência U5 — Fedora 44 (24/10/2026)

Em Debug, as matrizes `popular_apps_report_matrix`,
`popular_apps_recursive_report_matrix`, `popular_apps_native_matrix`,
`popular_apps_install_matrix` e `popular_apps_x86_installer_rejection_matrix`
passaram em série; a matriz confirma como resultado esperado a rejeição dos
quatro containers PE32 e que nenhum deles deixa payload ou cadastro. Os
smokes de cópia 7-Zip, extração real WinRAR, edição de arquivo Notepad++ em
X11 e Wayland, e GUI Rockstar passaram duas vezes no container Fedora com
Xvfb/Weston. Os smokes PuTTY de GUI e probe SSH local também passaram duas
vezes; o probe encerra com `GuestTimeout 72` após chegar a
`SSH_MSG_KEXDH_INIT`, que continua sendo uma limitação publicada. O smoke MSIX
Affinity passou duas vezes e não executa o app .NET.

O `--report` deixou de preparar prefixos, corrigindo a falha das matrizes em
`HOME` somente leitura. O orçamento do G HUB foi ampliado após a repetição
mostrar que 3 s expiravam antes de `ExitProcess(0)`. O smoke PuTTY passou a
aceitar a criação da janela registrada no trace quando a consulta X11 ocorre
depois que ela já foi fechada. Os logs integrais estão em
`/tmp/tl-u5-evidence/`. A verificação Sanitizers do driver C++ PuTTY e a
validação Ubuntu permanecem pendentes.

## Documentos temáticos

| Documento | Conteúdo |
|---|---|
| [`compatibilidade-aplicativos.md`](compatibilidade-aplicativos.md) | Aplicativos de teste, corpus popular, instalação e evidências operacionais. |
| [`compatibilidade-runtime.md`](compatibilidade-runtime.md) | Loader PE, imagem, imports, console, GUI, CRT, arquivos, recursos, segurança, COM e concorrência. |
| [`compatibilidade-backends.md`](compatibilidade-backends.md) | Políticas Rust/C++, MSIX/AppX, perfis, catálogo e backend Proton. |

A matriz temática deve ser atualizada junto com o diagnóstico quando uma
função, fixture ou aplicativo mudar de estado.
