# TradutorLinux

Runtime de compatibilidade Win32 para Linux. O projeto executa, de forma gradual
e documentada, um subconjunto de executáveis PE32+ x86-64 de console e GUI
experimental no Linux x86-64.

O objetivo de longo prazo é ampliar esse subconjunto para classes cada vez mais abrangentes de aplicativos Windows, sempre com testes, limitações publicadas e regressões reproduzíveis.

O estado de execução segue o [`ROADMAP.md`](ROADMAP.md): as etapas **R25–R29**
estão concluídas com evidências registradas — transporte SSH inicial do PuTTY
(R25), despacho polimórfico e cleanups C++ FH4 estendidos (R26), controles
comuns `SysListView32`/`COMCTL32` (R27), pacotes MSIX de grande porte e
validação de Affinity x64 (R28) e tolerância do leitor PE para diretórios em
seções virtuais UPX de `HWiNFO64.exe`/`Rufus_x64.exe` (R29). O histórico
anterior está em `feitos/ROADMAP-R1-R24.md` e `feitos/ROADMAP-LEGADO.md`. A
fila atual é a varredura de correções da
[auditoria 2026-10-18](docs/auditoria-2026-10-18.md), em andamento. Parser,
mapeamento, imports, console, runtime básico, diagnóstico, relatório de
cobertura, instaladores em prefixos e GUI Win32 experimental já têm entregas
validadas. O suporte continua restrito às aplicações, APIs e limitações
publicadas na matriz de compatibilidade.

Consulte [PROJETO.md](PROJETO.md) para visão e arquitetura e o [roadmap legado](feitos/ROADMAP-LEGADO.md) para o histórico dos marcos.

Para testar visualmente as aplicações GUI em uma sessão X11 real, consulte o
[guia de teste visual](docs/guia-visual.md). O smoke test automático usa
`Xvfb` e valida comportamento sem abrir uma janela visível.

A tela principal pode ser aberta com:

```bash
./build/debug/src/tradutorlinux_gui
```

O pacote Debian pode ser gerado com `cpack --config build/debug/CPackConfig.cmake -G DEB`
e instalado com `sudo apt install ./tradutorlinux_0.0.0_amd64.deb`. Depois da instalação,
o launcher aparece no menu de aplicativos como **TradutorLinux**.

Ela permite informar um `.exe`, analisar imports, executar o convidado e
acompanhar o diagnóstico diretamente na janela.

## Ambiente de desenvolvimento

O ambiente suportado inicialmente é Ubuntu 24.04 LTS x86-64, instalado nativamente ou em uma máquina virtual.

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git gdb llvm clang-tidy clang-format cppcheck gcc-mingw-w64-x86-64-win32
```

O pacote MinGW gera fixtures PE32+ para Windows; ele não é usado para compilar o runtime Linux.

## Build e testes

Na raiz do repositório:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Para validar memória indefinida e acessos inválidos durante os testes:

```bash
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize --output-on-failure
```

As fixtures são geradas em `build/<preset>/tests/samples/generated/`; não devem ser adicionadas ao Git.

## CLI atual

```text
tradutorlinux [--trace] [--report] <arquivo.exe>
tradutorlinux --help
tradutorlinux --version
```

`--trace` escreve diagnósticos somente em `stderr`. A saída padrão será reservada à futura saída do programa Windows. O contrato completo de trace e códigos de saída está em [docs/diagnostico.md](docs/diagnostico.md).

`--report` lista os imports e o estado de resolução sem mapear nem executar o
entry point. Retorna `0` quando todas as dependências estáticas foram resolvidas
e `5` quando há uma limitação de resolução conhecida. Isso não prova que o
aplicativo executa um fluxo funcional; essa conclusão exige execução e teste
registrados em [docs/compatibilidade.md](docs/compatibilidade.md).

## Qualidade

O projeto usa C++20, CMake, Ninja, GoogleTest, CTest, clang-tidy, cppcheck e sanitizers. O GitHub Actions executa build, testes e análise estática para pushes e pull requests.

## Licença e Documentação / License & Documentation

* **Licença / License:** [GNU Lesser General Public License v3.0 (LGPL-3.0)](LICENSE)
* **Diretrizes de contribuição / Contribution guidelines:** [AGENTS.md](AGENTS.md)
* **Visão do projeto / Project overview:** [PROJETO.md](PROJETO.md)
* **Roadmap:** [ROADMAP.md](ROADMAP.md)

