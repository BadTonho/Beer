<div align="center">
  <img src="beer.png" alt="Caneca de cerveja ilustrada, logo do TradutorLinux" width="180">

  <h1>TradutorLinux</h1>

  <p><strong>Runtime de compatibilidade Win32 para Linux</strong></p>

  <p>Execute, de forma progressiva e documentada, aplicativos Windows PE32+ x86-64<br>
  no Linux x86-64 — sem máquina virtual e sem emular a CPU.</p>
</div>

> Compatibilidade é publicada por aplicativo e por cenário. Consulte a [matriz](docs/compatibilidade.md) para ver o que foi testado e quais limitações permanecem.

## O projeto

O TradutorLinux carrega programas Windows de 64 bits e oferece as APIs Win32
implementadas pelo runtime. O objetivo é ampliar esse suporte por etapas
verificáveis, com diagnósticos reproduzíveis e regressões para cada capacidade.

O runtime ainda é experimental. A interface gráfica Win32 tem suporte mínimo e
experimental; outros recursos só são considerados suportados quando aparecem
na matriz de compatibilidade com evidências de teste.

## O que você pode fazer

- **Analisar imports sem executar:** `--report` mostra dependências resolvidas e limitações conhecidas; retorna `0` quando todas foram resolvidas e `5` quando há uma limitação conhecida.
- **Investigar uma execução:** `--trace` registra diagnósticos em `stderr`; a saída padrão fica reservada ao programa convidado.
- **Usar a interface gráfica do TradutorLinux:** informar um `.exe`, analisar imports e acompanhar a execução.
- **Consultar evidências:** a matriz separa resolução de imports, resultado da execução e nível funcional.

Para detalhes e limitações por aplicativo, consulte [`docs/compatibilidade.md`](docs/compatibilidade.md) e [`docs/compatibilidade-aplicativos.md`](docs/compatibilidade-aplicativos.md).

## Uso

```bash
# Ajuda da linha de comando
./build/debug/src/tradutorlinux --help

# Analisar imports sem iniciar o programa
./build/debug/src/tradutorlinux --report /caminho/para/programa.exe

# Executar com diagnóstico detalhado
./build/debug/src/tradutorlinux --trace /caminho/para/programa.exe

# Abrir a interface gráfica experimental
./build/debug/src/tradutorlinux_gui
```

`--report` não executa o programa e não comprova que seus fluxos funcionam.
Executáveis convidados rodam com os privilégios do usuário atual: o
TradutorLinux não é um sandbox.

O contrato completo da CLI, do trace e dos códigos de saída está em
[`docs/diagnostico.md`](docs/diagnostico.md).

### Gerar o pacote Debian

Depois de compilar, gere e instale o pacote localmente:

```bash
cpack --config build/debug/CPackConfig.cmake -G DEB
sudo apt install ./tradutorlinux_0.0.0_amd64.deb
```

Após a instalação, o launcher aparece no menu de aplicativos como
**TradutorLinux**.

## Compilar e desenvolver

O ambiente documentado é Ubuntu 24.04 LTS x86-64. Instale as ferramentas
necessárias:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git gdb llvm clang-tidy clang-format cppcheck gcc-mingw-w64-x86-64-win32
```

Configure e compile o preset de desenvolvimento:

```bash
cmake --preset debug
cmake --build --preset debug
```

Para executar a suíte de testes:

```bash
ctest --preset debug --output-on-failure
```

O MinGW é usado para gerar fixtures PE32+; ele não compila o runtime Linux.
As fixtures geradas ficam em `build/<preset>/tests/samples/generated/` e não
devem ser adicionadas ao Git.

## Documentação

| Documento | Conteúdo |
|---|---|
| [`PROJETO.md`](PROJETO.md) | Missão, arquitetura e princípios técnicos |
| [`ROADMAP.md`](ROADMAP.md) | Etapas atuais e critérios de saída |
| [`docs/compatibilidade.md`](docs/compatibilidade.md) | Matriz de compatibilidade e limitações |
| [`docs/diagnostico.md`](docs/diagnostico.md) | Trace, relatórios e códigos de saída |
| [`docs/guia-visual.md`](docs/guia-visual.md) | Testes visuais da GUI experimental |
| [`AGENTS.md`](AGENTS.md) | Diretrizes de contribuição e desenvolvimento |

## Escopo

O alvo atual é **PE32+ x86-64 em Linux x86-64**. PE32/x86, ARM, WOW64,
.NET, COM/ActiveX amplo, drivers, serviços Windows, anticheat, DirectX e GUI
Win32 ampla estão fora do escopo atual. A inclusão de novos recursos depende de
um aplicativo ou fixture-alvo, implementação, regressão e atualização da
matriz.

## Licença

TradutorLinux é distribuído sob a [GNU LGPL v3.0](LICENSE).
