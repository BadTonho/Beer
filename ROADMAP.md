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

- [ ] Permitir remover um cadastro pelo launcher Qt usando o contrato de
  `app remove`.
- [ ] Remover o `.desktop` correspondente quando o cadastro for removido pela
  CLI ou pela GUI.
- [ ] Preservar os arquivos instalados e o prefixo ao remover somente o
  cadastro.
- [ ] Cobrir cadastro, localização na biblioteca, execução e remoção em
  diretórios de configuração temporários.
- [ ] Tratar falhas de persistência sem exibir sucesso ou estado divergente.

**Saída:** GUI, CLI, catálogo e menu refletem o mesmo estado; os dados do
aplicativo permanecem intactos após remover o cadastro.

### U2 — Abrir arquivos pelo desktop Linux

**Objetivo:** permitir iniciar um aplicativo cadastrado com arquivos escolhidos
no gerenciador de arquivos ou em uma entrada do menu.

- [ ] Definir e documentar os argumentos de `app run` para um ou mais arquivos.
- [ ] Encaminhar argumentos sem shell intermediário e converter caminhos para
  a visão do prefixo do aplicativo.
- [ ] Registrar associações de arquivo somente para aplicativos e tipos
  escolhidos pela pessoa usuária.
- [ ] Cobrir espaços, caracteres Unicode e arquivos fora do prefixo.
- [ ] Verificar que o aplicativo-alvo abre o arquivo correto e que gravações
  chegam ao destino esperado.

**Saída:** ao menos um aplicativo cadastrado abre e salva um arquivo iniciado
no desktop, com os casos de caminho testados documentados.

### U3 — GUI confiável na sessão Linux

**Objetivo:** validar o ciclo de vida e a interação das janelas nos backends
gráficos existentes, com limites claros por ambiente.

- [ ] Manter X11/Xvfb como cenário automatizado independente.
- [ ] Criar cenário automatizado sob Wayland quando houver compositor de teste
  disponível.
- [ ] Validar criação, título, foco, redimensionamento, teclado, mouse e
  fechamento para um aplicativo real escolhido a partir da matriz.
- [ ] Corrigir somente diferenças observadas no fluxo-alvo e adicionar
  regressão para cada comportamento alterado.
- [ ] Registrar resultados e limitações de cada backend na matriz e no contrato
  de GUI.

**Saída:** pelo menos um fluxo real tem evidência reproduzível em cada backend
declarado como coberto.

### U4 — Fluxos cotidianos completos

**Objetivo:** transformar os smokes parciais existentes em tarefas verificadas
do início ao resultado observável.

- [ ] Priorizar cópia no 7-Zip File Manager, extração no WinRAR, edição e
  gravação no Notepad++ e geração de lista `.efu` no Everything.
- [ ] Para cada tarefa, verificar entradas, interação ou argumentos, artefato
  final, código de saída e encerramento.
- [ ] Repetir o cenário em prefixo limpo e após reiniciar o aplicativo quando
  isso fizer parte do fluxo.
- [ ] Manter comportamento específico de aplicativo em `compat/apps/<app-id>/`
  e ativado somente por seleção explícita.
- [ ] Promover na matriz somente os fluxos que passaram integralmente; imports
  resolvidos e janela aberta, isoladamente, não bastam.

**Saída:** pelo menos três tarefas do portfólio são repetíveis de ponta a ponta
e têm evidência e limites publicados.

### U5 — Instalação e diagnóstico para uso contínuo

**Objetivo:** tornar a instalação e a recuperação de problemas claras para
quem usa o aplicativo no dia a dia.

- [ ] Definir um caminho reproduzível de instalação e inicialização do
  TradutorLinux em Linux x86-64.
- [ ] Documentar requisitos do host, instalação de aplicativos, prefixos,
  biblioteca, atualização e remoção de cadastro.
- [ ] Garantir que erros comuns indiquem aplicativo, etapa, módulo ou símbolo
  envolvido e a próxima limitação conhecida.
- [ ] Validar instruções em uma instalação limpa e manter exemplos alinhados
  com os comandos realmente disponíveis.

**Saída:** uma pessoa consegue instalar, cadastrar, abrir, diagnosticar e
remover um aplicativo seguindo as instruções publicadas.

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
