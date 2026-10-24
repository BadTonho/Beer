# Instalação e uso no Fedora

Este guia cobre Fedora 44 x86-64. A validação equivalente no Ubuntu está
pendente para quando esse ambiente estiver disponível.

## Requisitos para executar o pacote

- Linux x86-64.
- Para o launcher visual, uma sessão gráfica X11 ou Wayland. O pacote inclui o
  launcher Qt e declara no RPM as bibliotecas compartilhadas necessárias.

As dependências compartilhadas do runtime e do launcher são registradas nos
metadados RPM pelo detector do CPack. O pacote também declara
`desktop-file-utils`, que fornece `update-desktop-database` para manter o menu
sincronizado quando cadastros são adicionados ou removidos. O DNF instala essas
dependências junto com o pacote.

## Compilar e gerar o RPM

O preset Debug requer CMake 3.28 ou superior, Ninja, compilador C/C++20, Qt 6
Widgets, zlib de desenvolvimento, MinGW x64 e as ferramentas `llvm-readobj` e
`llc` para gerar as fixtures PE. Para compilar o launcher e os drivers de teste,
instale também os headers X11/XTest e Wayland. A geração RPM requer `rpm-build`:

```bash
sudo dnf install cmake ninja-build gcc-c++ qt6-qtbase-devel zlib-devel \
  mingw64-gcc llvm wayland-devel libxkbcommon-devel libX11-devel libXtst-devel \
  rpm-build
cmake --preset debug
cmake --build --preset debug --target tradutorlinux tradutorlinux_gui --parallel 2
cpack -G RPM --config build/debug/CPackConfig.cmake -B /tmp/tradutorlinux-rpms
```

Smokes gráficos usam `xorg-x11-server-Xvfb`; o teste Wayland também requer
Weston. Os nomes dos pacotes de desenvolvimento podem variar entre versões do
Fedora.

## Instalar ou atualizar o TradutorLinux

Instale o RPM gerado pelo projeto; o DNF resolve as dependências do pacote:

```bash
sudo dnf install /tmp/tradutorlinux-rpms/*.rpm
```

O menu do desktop terá **TradutorLinux**. Também é possível abrir o launcher
com `tradutorlinux_gui`, usar a CLI com `tradutorlinux` e verificar o host com:

```bash
tradutorlinux doctor
tradutorlinux doctor --json
```

Para atualizar usando um RPM de versão mais nova:

```bash
sudo dnf upgrade /tmp/tradutorlinux-rpms/*.rpm
```

## Analisar, instalar e abrir aplicativos

Antes de instalar um executável, consulte seus requisitos estáticos:

```bash
tradutorlinux --report /caminho/para/aplicativo.exe
```

Para um instalador PE32+ x86-64 compatível:

```bash
tradutorlinux install /caminho/para/instalador.exe --name "Nome do aplicativo"
tradutorlinux app list
tradutorlinux app run <id> --trace
```

Se o instalador concluir mas não houver um único executável novo identificável,
o comando retorna `6` (`InstallPending`); escolha o executável dentro do
prefixo usando `--app-exe`. Para um aplicativo portátil, cadastre seu
executável com `tradutorlinux app add /caminho/aplicativo.exe --name "Nome"`.
O aplicativo também pode ser aberto pela biblioteca do launcher.

`doctor` verifica o ambiente Linux. Para investigar um aplicativo, use
`--report` antes da execução e `--trace` durante ela; o trace vai para
`stderr`, preservando `stdout` para a saída do programa. A matriz em
[`compatibilidade.md`](compatibilidade.md) registra os resultados e limitações
por aplicativo. O runtime não é uma sandbox: aplicativos convidados herdam os
privilégios do usuário atual.

## Biblioteca, configurações e prefixos

- Catálogo: `$XDG_CONFIG_HOME/tradutorlinux/library.json`; sem
  `XDG_CONFIG_HOME`, usa `$HOME/.config/tradutorlinux/library.json`.
- Prefixo padrão dos aplicativos: `$HOME/.tradutorlinux/prefixes/<id>/`.
- Dados de aplicativos convidados ficam dentro do prefixo, em `drive_c`, e são
  separados por ID.
- Atalhos individuais: `$XDG_DATA_HOME/applications`; sem essa variável,
  `$HOME/.local/share/applications`.

`tradutorlinux app remove <id>` remove o cadastro e o atalho, mas preserva o
executável e o prefixo. Isso permite recadastrar o aplicativo sem reinstalá-lo.

## Remover

Remova primeiro os cadastros que não deseja manter e depois o pacote:

```bash
tradutorlinux app list
tradutorlinux app remove <id>
sudo dnf remove tradutorlinux
```

A remoção do pacote não apaga os prefixos nem arquivos pessoais. Apague um
prefixo em `$HOME/.tradutorlinux/prefixes/<id>/` somente se também quiser
remover os dados daquele aplicativo.

## Diagnóstico rápido

- `doctor` retorna arquitetura ou display não suportado: confirme que o host é
  Linux x86-64 e que há sessão X11/Wayland ativa.
- `--report` lista módulo ou símbolo não resolvido: veja a recomendação e a
  matriz; `result: supported` significa apenas que os imports analisados foram
  resolvidos, não que o fluxo principal funcione.
- `app run` encerra com erro: repita com `--trace` e guarde o código de saída.
  O primeiro componente e evento de nível `error` indicam a etapa; consulte
  [`diagnostico.md`](diagnostico.md) para códigos e próximos limites conhecidos.
- Falha de instalação: execute `install` com `--trace` e confira o campo
  `stage` (`input`, `package-parse`, `package-extract`, `setup` ou `catalog`).

Os códigos de saída, os componentes e os formatos do trace estão definidos em
[`diagnostico.md`](diagnostico.md) e
[`diagnostico-backends.md`](diagnostico-backends.md).
