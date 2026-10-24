# Roadmap de uso diário — TradutorLinux

## Objetivo

Tornar confiável o uso, no desktop Linux, dos aplicativos PE32+ x86-64 que o
TradutorLinux já executa parcialmente. O ciclo prioriza tarefas completas de
ponta a ponta: instalar, cadastrar, abrir, trabalhar com arquivos, diagnosticar
problemas e remover um aplicativo da biblioteca.

O roadmap não declara compatibilidade geral com aplicativos Windows. Cada
fluxo promovido precisa de um resultado verificável, repetição em ambiente
controlado e limitações registradas na matriz.

## Base atual

- O launcher Qt oferece biblioteca, busca, análise, execução, instalação,
  cadastro e diagnóstico do host.
- O catálogo CLI oferece `app add`, `app list`, `app run` e `app remove`, usa
  prefixos por aplicativo e cria entradas `.desktop` ao cadastrar.
- Há backends gráficos X11 e Wayland e smokes de GUI para aplicativos do
  corpus; a matriz descreve o estado e os limites de cada cenário.
- Existem fluxos parciais de cópia no 7-Zip File Manager, extração no WinRAR,
  GUI de Notepad++ e Everything, e interação inicial com o Rockstar Games
  Launcher.

Use [`docs/compatibilidade.md`](docs/compatibilidade.md) e suas matrizes
temáticas como fonte do estado técnico atual. Os roadmaps em `feitos/` são
histórico e não definem trabalho novo.

## Fila de trabalho

### U1 — Biblioteca e atalhos consistentes

**Objetivo:** administrar os cadastros pela GUI ou CLI sem deixar estado
desatualizado no catálogo ou no menu do desktop.

- [x] Permitir remover um cadastro pelo launcher Qt usando o contrato de
  `app remove`.
- [x] Remover o `.desktop` correspondente quando o cadastro for removido pela
  CLI ou pela GUI.
- [x] Preservar os arquivos instalados e o prefixo ao remover somente o
  cadastro.
- [x] Cobrir cadastro, localização na biblioteca, execução e remoção em
  diretórios de configuração temporários.
- [x] Tratar falhas de persistência sem exibir sucesso ou estado divergente.

**Saída:** GUI, CLI, catálogo e menu refletem o mesmo estado; os dados do
aplicativo permanecem intactos após remover o cadastro.

### U2 — Abrir arquivos pelo desktop Linux

**Objetivo:** permitir iniciar um aplicativo cadastrado com arquivos escolhidos
no gerenciador de arquivos ou em uma entrada do menu.

- [x] Definir e documentar os argumentos de `app run` para um ou mais arquivos.
- [x] Encaminhar argumentos sem shell intermediário e converter caminhos para
  a visão do prefixo do aplicativo.
- [x] Registrar associações de arquivo somente para aplicativos e tipos
  escolhidos pela pessoa usuária.
- [x] Cobrir espaços, caracteres Unicode e arquivos fora do prefixo.
- [x] Verificar que o aplicativo-alvo abre o arquivo correto e que gravações
  chegam ao destino esperado.

**Saída:** ao menos um aplicativo cadastrado abre e salva um arquivo iniciado
no desktop, com os casos de caminho testados documentados.

### U3 — GUI confiável na sessão Linux

**Objetivo:** validar o ciclo de vida e a interação das janelas nos backends
gráficos existentes, com limites claros por ambiente.

- [x] Manter X11/Xvfb como cenário automatizado independente.
- [x] Criar cenário automatizado sob Wayland com Weston aninhado em Xvfb.
- [x] Validar criação, título, foco, redimensionamento, teclado, mouse e
  fechamento para Notepad++ em X11 e Wayland.
- [x] Corrigir diferenças observadas e adicionar regressão para cada
  comportamento alterado.
- [x] Registrar resultados e limites de cada backend na matriz e no contrato
  de GUI.

**Evidência U3 (22/10/2026):** `x11_popup_smoke`,
`notepadpp_file_open_save_smoke`, `notepadpp_wayland_gui_smoke` e a regressão
`Win32GuiTest.DefWindowProcHandlesTextMessagesAndAltF4ForTopLevel` passaram nos
presets Debug e Sanitizers. Os smokes confirmam gravação no arquivo original e
encerramento normal com exit code `0`. O cenário Wayland usa Weston sobre Xvfb;
uma sessão Wayland nativa permanece fora desta evidência.

**Saída:** fluxo real reproduzível nos cenários X11/Xvfb e
Wayland/Weston/Xvfb, com limites publicados para cada ambiente.

### U4 — Fluxos cotidianos completos

**Objetivo:** transformar os smokes parciais existentes em tarefas verificadas
do início ao resultado observável.

- [x] Priorizar cópia no 7-Zip File Manager, extração no WinRAR, edição e
  gravação no Notepad++ e geração de lista `.efu` no Everything.
- [x] Para cada tarefa, verificar entradas, interação ou argumentos, artefato
  final, código de saída e encerramento.
- [x] Repetir o cenário em prefixo limpo e após reiniciar o aplicativo quando
  isso fizer parte do fluxo.
- [x] Manter comportamento específico de aplicativo em `compat/apps/<app-id>/`
  e ativado somente por seleção explícita.
- [x] Promover na matriz somente os fluxos que passaram integralmente; imports
  resolvidos e janela aberta, isoladamente, não bastam.

**Saída:** pelo menos três tarefas do portfólio são repetíveis de ponta a ponta
e têm evidência e limites publicados.

**Evidência U4 (24/10/2026):** os CTests `seven_zip_gui_smoke`,
`seven_zip_subdir_gui_smoke`, `winrar_extract_real_smoke`,
`winrar_723_extract_real_smoke`, `notepadpp_real_gui_smoke` e
`notepadpp_wayland_gui_smoke` passaram duas vezes em série nos presets Debug e
Sanitizers. A execução U4 da matriz de instaladores também registrou o `.efu`
do Everything. Na avaliação U5, porém, os instaladores disponíveis para
7-Zip, Notepad++, Everything e RTSS foram confirmados como PE32 x86; o corpus
atual não inclui um `everything.exe` autônomo e o resultado `.efu` não pôde ser
reproduzido. A matriz vigente verifica e documenta a rejeição desses
containers. O WinRAR cria um `.rar` com `Rar.exe`, extrai com `UnRAR.exe` via
catálogo e compara os bytes; Notepad++ abre, edita, grava no arquivo original e
fecha em X11 e Weston aninhado. O build Sanitizers usou
`ASAN_OPTIONS=detect_leaks=0` porque LeakSanitizer não inicia sob ptrace neste
ambiente; AddressSanitizer e UBSan permaneceram ativos.

### U5 — Instalação Fedora e avaliação do corpus

**Objetivo:** fornecer um pacote RPM Fedora instalável e registrar os resultados
de análise, instalação e inicialização dos produtos identificáveis do corpus.

- [x] Gerar RPM por CPack sem remover o gerador DEB e instalar o pacote no
  Fedora 44 descartável, conferindo dependências, binários, ícone e menu.
- [x] Documentar requisitos, compilação, instalação, atualização, diagnóstico,
  biblioteca, prefixos e remoção de cadastro para Fedora.
- [x] Analisar e registrar os 19 produtos identificados no corpus; cobrir
  instalação e inicialização quando o formato e o runtime permitirem, sem
  promover rejeições ou limitações conhecidas.
- [x] Corrigir a análise `--report` para não preparar prefixos e ajustar limites
  e critérios dos smokes quando a primeira execução expôs problemas do método.
- [ ] Validar o driver C++ PuTTY no preset Sanitizers ou no CI.
- [ ] Validar as instruções no Ubuntu quando esse ambiente estiver disponível.

**Evidência parcial (24/10/2026):** Debug, RPM Fedora e as matrizes/smokes
selecionados passaram em ambiente descartável. A revisão Sanitizers e Ubuntu
continuam pendentes; não declarar U5 concluída antes dessas validações.

## Regras para avançar os marcos

- Trabalhar somente na etapa pendente ou em dependência direta dela.
- Cada API Win32 nova precisa de fluxo-alvo existente e teste de regressão.
- Manter o runtime genérico; isolar adaptações específicas em extensões
  explícitas por aplicativo, conforme os contratos do projeto.
- Atualizar contratos e matriz de compatibilidade quando o comportamento ou o
  estado de uma aplicação mudar.
- Marcar uma tarefa concluída somente com código ou ferramenta, evidência
  reproduzível e validação exigida pelas regras do projeto.
- Preservar Linux x86-64 e PE32+ x86-64. Ampliar arquitetura ou incluir classes
  atualmente fora do escopo exige decisão explícita e critérios próprios.

## Critério de conclusão do ciclo

U1–U5 têm evidência reproduzível; catálogo e menu permanecem consistentes; pelo
menos três fluxos cotidianos são concluídos nos ambientes declarados; e as
instruções publicadas permitem instalação, uso e diagnóstico sem extrapolar a
matriz de compatibilidade.

## Histórico

O roadmap anterior de experiência desktop foi substituído neste ciclo. Os
roadmaps R25–R29 e R1–R24 continuam preservados em
[`feitos/ROADMAP-R25-R29.md`](feitos/ROADMAP-R25-R29.md) e
[`feitos/ROADMAP-R1-R24.md`](feitos/ROADMAP-R1-R24.md); o histórico fundacional
está em [`feitos/ROADMAP-LEGADO.md`](feitos/ROADMAP-LEGADO.md).
