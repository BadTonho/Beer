# Arquitetura: Instaladores e Biblioteca de Aplicativos

Este documento detalha os contratos técnicos do subsistema de **Prefixos de Ambiente** e da **Biblioteca de Aplicativos (Catalog/Launcher)** do TradutorLinux.

---

## 1. Subsistema de Prefixos (`prefix`)

Localização:
- Header: [include/tradutorlinux/prefix/prefix.hpp](file:///c:/Users/Admin/Desktop/ProjetosCode/Linux/TradutorLinux/include/tradutorlinux/prefix/prefix.hpp)
- Implementação: [src/prefix/prefix.cpp](file:///c:/Users/Admin/Desktop/ProjetosCode/Linux/TradutorLinux/src/prefix/prefix.cpp)

### Estrutura de Diretórios
Execuções diretas legadas usam `~/.tradutorlinux/`. Cada nova instalação e cada
novo cadastro persistente usam, por padrão, um prefixo exclusivo em
`~/.tradutorlinux/prefixes/<id>/`:
```text
~/.tradutorlinux/prefixes/<id>/
└── drive_c/
    ├── Program Files/
    ├── Program Files (x86)/
    ├── users/
    │   └── guest/
    │       └── AppData/
    │           ├── Local/
    │           └── Roaming/
    └── windows/
        ├── system32/
        └── temp/
```

Entradas antigas cujo `prefix_path` ainda é o prefixo compartilhado preservam
o `working_directory` externo já salvo. Entradas novas instaladas no prefixo
usam o diretório do executável dentro do `drive_c`; cadastros de executáveis
externos preservam o diretório original para localizar seus arquivos auxiliares.

### Resolução de Caminhos Windows
- `resolve_windows_path("C:\\Program Files\\App\\app.exe")`: Resolve para `<prefix>/drive_c/Program Files/App/app.exe`.
- Normaliza barras (`\\` → `/`) e drives virtuais (`C:`).
- `to_windows_path(linux_path)`: Converte caminhos dentro do prefixo para
  `C:\...`; um arquivo externo, como o setup selecionado, é exposto como
  `Z:\...`.
- O runtime fixa o prefixo ativo antes de iniciar o convidado. Caminhos Win32,
  CRT, diretório atual, `TEMP`/`TMP`, `APPDATA`, `LOCALAPPDATA` e
  `USERPROFILE` usam esse contexto. Isso é isolamento funcional de dados, não
  uma sandbox: `Z:` ainda representa o sistema de arquivos do hospedeiro.

`initialize_prefix` materializa os drives como links explícitos dentro de
`dosdevices`: `c:` aponta para `../drive_c` e `z:` aponta para `/`. A resolução
canoniza caminhos `C:` e rejeita qualquer escape; o contrato é protegido por
`PrefixTest.DriveLinksAreExplicitAndKeepCDriveConfined`. O link `Z:` permanece
deliberadamente externo e nunca deve ser tratado como mecanismo de segurança.

Em execuções diretas e em aplicativos cadastrados fora do `drive_c`, o processo
filho usa o diretório do executável como diretório de trabalho. Assim, recursos
auxiliares relativos (`Lang`, DLLs, ícones e configurações) são procurados ao
lado do programa. Caminhos `C:` permanecem confinados ao `drive_c`; tentativas
com `..` que escapem do prefixo são rejeitadas.

---

## 2. Subsistema de Biblioteca / Catálogo (`catalog`)

Localização:
- Header: [include/tradutorlinux/catalog/app_catalog.hpp](file:///c:/Users/Admin/Desktop/ProjetosCode/Linux/TradutorLinux/include/tradutorlinux/catalog/app_catalog.hpp)
- Implementação: [src/catalog/app_catalog.cpp](file:///c:/Users/Admin/Desktop/ProjetosCode/Linux/TradutorLinux/src/catalog/app_catalog.cpp)

### Armazenamento
O catálogo é persistido em formato JSON em `~/.config/tradutorlinux/library.json`.

```json
{
  "version": 1,
  "apps": [
    {
      "id": "notepad_plus_plus",
      "name": "Notepad++",
      "executable_path": "/home/user/.tradutorlinux/drive_c/Program Files/Notepad++/notepad++.exe",
      "prefix_path": "/home/user/.tradutorlinux",
      "icon_path": "",
      "working_directory": "/home/user/.tradutorlinux/drive_c/Program Files/Notepad++",
      "created_at": "2026-08-19T12:00:00Z",
      "args": []
    }
  ]
}
```

---

## 3. Interface de Linha de Comando (CLI)

| Comando | Descrição |
|---|---|
| `tradutorlinux <app.exe>` | Execução direta de um executável PE32+. |
| `tradutorlinux install <setup.exe|package.msix> [--name <Nome>] [--prefix <dir>] [--app-exe <caminho>]` | Executa um setup PE32+ ou instala um pacote MSIX/AppX nativo no prefixo exclusivo. Só cadastra após o exit `0` do setup; pacotes selecionam o PE declarado no manifesto. |
| `tradutorlinux app list` | Lista todos os aplicativos cadastrados na biblioteca. |
| `tradutorlinux app run <id_ou_nome> [opções] [argumentos...] [-- <arquivo>...]` | Executa um aplicativo cadastrado; arquivos após `--` são encaminhados como caminhos de arquivo. |
| `tradutorlinux app add <app.exe> [--name <Nome>] [--prefix <dir>] [--id <id>]` | Cadastra manualmente um executável na biblioteca. Sem `--prefix`, cria prefixo exclusivo. |
| `tradutorlinux app remove <id>` | Remove o cadastro e o atalho individual do menu. O executável instalado e o prefixo são preservados. |
| `tradutorlinux app associate <id> <add|remove> <mime-type>` | Adiciona ou remove um MIME type anunciado pelo atalho do aplicativo. |

Em `app run`, argumentos comuns antes de `--` permanecem separados e na ordem
fornecida. Cada caminho após `--` é resolvido a partir do diretório atual de
quem iniciou o runtime, precisa apontar para um arquivo regular existente e é
convertido para a visão do prefixo: `C:\...` dentro de `drive_c` e `Z:\...`
fora dele. O runtime inicia o convidado sem shell intermediário. Vários arquivos,
espaços e caminhos UTF-8 são encaminhados como argumentos distintos.

As associações opt-in são guardadas no campo `MimeType` do atalho individual;
`Exec` inclui `%F` como argumento de arquivo isolado uma única vez. O launcher
Qt permite incluir ou remover MIME types: ao incluir, informa-se uma extensão
e `QMimeDatabase` resolve o tipo. O comando atualiza `mimeinfo.cache`, sem
escrever `mimeapps.list` nem alterar o aplicativo padrão. Se a atualização do
cache falhar, o runtime tenta restaurar o atalho anterior e comunica quando a
restauração também falha.

Os atalhos individuais ficam em `$XDG_DATA_HOME/applications`; quando a
variável não estiver definida, o destino é `$HOME/.local/share/applications`.
Se o atalho do ID já estiver ausente, `app remove` ainda pode remover o
cadastro. Falha ao remover o atalho mantém o catálogo intacto. A gravação do
catálogo usa arquivo temporário e substituição atômica; se ela falhar após a
remoção do atalho, o comando tenta restaurá-lo e retorna erro em vez de
informar sucesso. Se a restauração também falhar, o diagnóstico identifica a
falha parcial.

`app run` usa o runtime próprio por padrão. Um perfil schema 3 pode selecionar
explicitamente `backend.kind="proton"`; nesse caso, o launcher Proton é
validado pela configuração externa em `backends.json` e o aplicativo é
executado em um prefixo persistente separado dentro do prefixo cadastrado.
Ausência ou invalidez do Proton solicitado é erro explícito, não fallback
silencioso. Execução direta não seleciona Proton.

### Resultado de `install`

Antes de executar o setup, o CLI registra os PE32+ AMD64 já presentes em
`drive_c`. Após um exit `0`, ele procura candidatos novos ou alterados:

- um candidato é cadastrado automaticamente;
- `--app-exe` é validado como arquivo PE32+ AMD64 contido no `drive_c`;
- zero ou mais de um candidato retornam `6` (`InstallPending`), preservam o
  prefixo e não criam entrada de catálogo;
- falha, timeout ou import não suportado nunca cadastram o setup nem um
  executável parcial.

Com `--trace`, os eventos `install` em `stderr` publicam `prepared`,
`extracted`, `candidate`, `registered`, `pending` ou `failed`, sempre com
prefixo e ID. Para pacotes, `extracted` informa o PE selecionado e os estágios
`package-parse`, `package-extract` e `package-executable` distinguem as
falhas de estrutura, extração e arquitetura.

### Pacotes MSIX/AppX

O suporte atual é deliberadamente restrito a `.msix`/`.appx` ZIP que tenham
`AppxManifest.xml` e um executável PE32+ x86-64 nativo. A extração percorre o
central directory, verifica tamanhos, CRC, método `stored`/`deflate`, nomes
relativos, symlinks e colisões antes de escrever no prefixo; o destino precisa
ser novo ou vazio. O executável indicado por `Application Executable` é
validado novamente antes do cadastro.

Bundles, assemblies .NET/Mono, assinatura Authenticode e outras arquiteturas
não fazem parte desse contrato. A fixture `native-fixture.msix` e o CTest
`integration_msix_install` cobrem relatório, instalação, catálogo e `app run`.

---

## 4. Interface Gráfica do Launcher (`tradutorlinux_gui`)

A interface usa Qt6 e está implementada em
[`src/gui/main_window.cpp`](../../src/gui/main_window.cpp), com entrada em
[`src/gui/qt_main.cpp`](../../src/gui/qt_main.cpp). O alvo continua sendo
`tradutorlinux_gui`.

O launcher exibe a biblioteca, filtra aplicativos, permite selecionar um
executável pelo `QFileDialog` ou pelo campo de caminho e disponibiliza ações
assíncronas:

- **Analisar**: executa o relatório estático (`--report`) e captura o
  diagnóstico;
- **Executar**: inicia o programa com `--trace` sem bloquear a janela;
- **Instalar**: chama `install` com o nome opcional informado. Ao receber
  `registered`, atualiza a biblioteca; com vários `candidate`, pede a escolha
  e finaliza o cadastro sem executar novamente o setup;
- **Cadastrar**: salva ou atualiza a entrada no `library.json`;
- **Remover cadastro**: após confirmação, remove o cadastro e o atalho do menu,
  mantendo os arquivos instalados e o prefixo; em falha, recarrega o estado
  persistido e apresenta erro;
- **Limpar**: restaura o formulário sem apagar a biblioteca.

A execução usa `QProcess`, procura o binário `tradutorlinux` no mesmo diretório
do launcher e separa stdout de stderr no console visual. O contrato completo,
os limites e os testes estão em
[`guia-ui-qt6.md`](guia-ui-qt6.md).

## 5. Pacote Debian

O CMake instala `tradutorlinux` e `tradutorlinux_gui` em `/usr/bin`, o launcher
`tradutorlinux.desktop` em `/usr/share/applications` e o ícone SVG no tema
Hicolor. O pacote é gerado por CPack com o gerador `DEB`.
