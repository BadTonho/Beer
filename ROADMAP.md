# Roadmap de Experiência Desktop — TradutorLinux

## Direção

Este roadmap orienta o próximo ciclo do TradutorLinux: tornar mais simples e
confiável usar no desktop Linux os aplicativos Win32 **PE32+ x86-64** que o
projeto já executa parcialmente. O foco é completar tarefas de usuário, integrar
os aplicativos à sessão Linux e publicar evidências comparáveis.

O objetivo é competir em experiência e em fluxos selecionados. Este ciclo não
busca paridade geral com Wine nem amplia a arquitetura suportada.

## Escopo deste ciclo

- Manter Linux x86-64 e executáveis PE32+ x86-64.
- Trabalhar com aplicativos já presentes no corpus e na matriz, incluindo
  7-Zip, WinRAR, Notepad++, Everything e Rockstar Games Launcher.
- Usar interfaces e serviços compartilhados para toda capacidade genérica.
- Manter extensões específicas em `compat/apps/<app-id>/`, somente com seleção
  explícita e contrato documentado.
- Comparar fluxos de usuário reproduzíveis com uma versão identificada do Wine.

PE32/x86, ARM, WOW64, .NET, COM amplo, drivers, serviços Windows, anti-cheat,
DirectX nativo, áudio e GPU continuam fora deste ciclo. Incluí-los exigirá uma
decisão de escopo e um roadmap próprio.

## Base já implementada

Os próximos marcos partem destas capacidades existentes; não as reabrem como
trabalho novo:

- O launcher Qt oferece biblioteca e busca, seleção de executável, análise,
  execução, instalação, cadastro e diagnóstico do host.
- O catálogo CLI oferece `app add`, `app list`, `app run` e `app remove`, usa
  prefixos por aplicativo e gera entradas `.desktop` ao cadastrar.
- O runtime tem backends gráficos Linux X11 e Wayland, controles Win32
  experimentais e smokes reais de GUI para aplicativos do corpus.
- A matriz já registra tarefas concretas, como operação limitada de cópia no
  7-Zip File Manager, extração WinRAR, inicialização e interação do launcher
  Rockstar, inicialização GUI do Notepad++ e indexação CLI do Everything.

Estados e limites continuam registrados em
[`docs/compatibilidade.md`](docs/compatibilidade.md),
[`docs/compatibilidade-aplicativos.md`](docs/compatibilidade-aplicativos.md) e
[`docs/compatibilidade-runtime.md`](docs/compatibilidade-runtime.md).

## Regras de execução

Cada marco deve produzir implementação ou ferramenta de validação, testes
automatizados ou um critério manual reproduzível, atualização dos contratos
afetados e registro honesto na matriz de compatibilidade. Imports resolvidos,
janela criada ou encerramento limpo isoladamente não comprovam uma tarefa de
uso diário.

Novas APIs Win32 precisam de um fluxo-alvo existente e regressão. Comportamento
específico de aplicativo não pode entrar nos módulos comuns. Nenhum marco pode
declarar suporte sem verificar o resultado produzido pelo aplicativo.

## Fila de trabalho

### ED1 — Comparação reproduzível de fluxos com Wine

**Objetivo:** criar uma linha de base que indique em quais tarefas o
TradutorLinux já entrega uma experiência competitiva e onde o runtime ainda
falha.

- [ ] Selecionar três fluxos GUI/desktop já cobertos parcialmente pela matriz,
  preferindo tarefas verificáveis em arquivos ou estado observável.
- [ ] Fixar as versões dos executáveis-alvo, do TradutorLinux e do Wine usados
  na comparação.
- [ ] Executar cada fluxo em ambiente limpo, com configuração e prefixo
  separados, registrando preparação, interação, resultado, tempo e limitações.
- [ ] Criar um relatório ou script reproduzível que mantenha os resultados do
  Wine fora dos testes obrigatórios quando Wine não estiver instalado.
- [ ] Publicar os resultados sem transformar uma pontuação agregada em alegação
  de compatibilidade geral.

**Saída:** protocolo e primeira comparação versionados; cada resultado pode ser
repetido no mesmo ambiente.

### ED2 — Ciclo de vida completo da biblioteca desktop

**Objetivo:** permitir que a pessoa administre os aplicativos pela interface
gráfica e manter catálogo e menu do Linux sincronizados.

- [ ] Adicionar remoção de cadastro no launcher Qt, usando o mesmo contrato do
  comando `app remove`.
- [ ] Remover a entrada `.desktop` correspondente quando o cadastro for
  removido pelo CLI ou pelo launcher.
- [ ] Manter os arquivos instalados e o prefixo ao remover o cadastro; remoção
  de dados fica fora deste marco.
- [ ] Cobrir instalar, cadastrar, localizar na biblioteca, executar, remover e
  confirmar ausência no catálogo e no menu em diretórios de configuração
  temporários.
- [ ] Tratar falhas de gravação do catálogo ou remoção da entrada de desktop sem
  deixar a interface mostrar um estado concluído incorreto.

**Saída:** o ciclo cadastro/execução/remoção funciona pela GUI e CLI, com
catálogo e atalhos consistentes e dados do prefixo preservados.

### ED3 — Abrir arquivos Linux com aplicativos Windows cadastrados

**Objetivo:** integrar os aplicativos selecionados ao fluxo de abrir arquivos
do desktop Linux.

- [ ] Definir o contrato de argumentos de `app run` para um ou mais arquivos,
  incluindo espaços, Unicode e caminhos fora do prefixo.
- [ ] Passar argumentos sem shell intermediário e convertê-los para caminhos
  visíveis ao convidado segundo o contrato do prefixo.
- [ ] Permitir que a entrada `.desktop` encaminhe arquivos somente para os
  aplicativos e tipos escolhidos pela pessoa usuária.
- [ ] Adicionar uma fixture de regressão para argumentos e uma integração com
  um aplicativo existente do corpus, verificando que o arquivo certo foi
  aberto e que gravações chegam ao destino esperado.
- [ ] Documentar os tipos de arquivo e limites efetivamente testados.

**Saída:** pelo menos um aplicativo cadastrado abre e salva um arquivo iniciado
no desktop Linux, com caminhos Unicode e com espaços protegidos por teste.

### ED4 — GUI convidada confiável em X11 e Wayland

**Objetivo:** evoluir os backends existentes usando problemas observados em
fluxos reais, sem declarar equivalência Win32 gráfica geral.

- [ ] Criar um cenário automatizado de aplicativo real sob Wayland quando houver
  compositor de teste disponível; manter X11/Xvfb como cenário independente.
- [ ] Validar ciclo de vida, título, fechamento, foco, redimensionamento,
  teclado e mouse no aplicativo-alvo escolhido.
- [ ] Acrescentar clipboard, escala DPI, diálogos ou outras integrações somente
  quando um fluxo-alvo exigir o comportamento e houver fixture de regressão.
- [ ] Registrar diferenças por backend na matriz e no contrato de GUI.

**Saída:** ao menos um fluxo real tem resultado reproduzível em X11 e Wayland;
limitações por backend aparecem no trace e na documentação.

### ED5 — Promover fluxos cotidianos por evidência

**Objetivo:** promover aplicativos existentes somente depois que tarefas
representativas forem repetíveis do início ao fim.

- [ ] Priorizar fluxos existentes do 7-Zip File Manager, WinRAR, Notepad++ e
  Everything; não adicionar aplicativos ao corpus apenas para aumentar a lista.
- [ ] Para cada fluxo, verificar entrada, interação, arquivo ou estado final,
  encerramento, repetição em prefixo limpo e comportamento após reinício.
- [ ] Executar os mesmos passos na versão fixada do Wine definida em ED1 e
  publicar diferenças e limitações.
- [ ] Atualizar nível funcional na matriz somente para as tarefas que passaram;
  manter `--report`, execução e uso diário como dimensões distintas.
- [ ] Preparar uma versão empacotada com instruções claras para instalar,
  cadastrar, abrir, diagnosticar e remover aplicativos.

**Saída:** pelo menos três fluxos cotidianos do portfólio têm evidência de
integração repetível e comparação publicada. Isso não implica suporte geral aos
aplicativos nem paridade com Wine.

## Critério de conclusão do ciclo

O ciclo termina quando ED1–ED5 têm evidência reproduzível, o launcher e o menu
Linux mantêm o estado dos aplicativos de forma consistente, pelo menos três
fluxos cotidianos passam em X11 e Wayland conforme sua cobertura declarada, e a
matriz publica limitações e comparação sem extrapolar os resultados.

## Histórico

O roadmap anterior, cobrindo as etapas R25–R29 e a rodada de corpus de setembro
de 2026, foi preservado em
[`feitos/ROADMAP-R25-R29.md`](feitos/ROADMAP-R25-R29.md). O histórico anterior
continua em [`feitos/ROADMAP-R1-R24.md`](feitos/ROADMAP-R1-R24.md) e
[`feitos/ROADMAP-LEGADO.md`](feitos/ROADMAP-LEGADO.md).
