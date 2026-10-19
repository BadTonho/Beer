# Testes específicos por aplicativo

Smokes, fixtures e cenários de interação de aplicativos reais ficam nesta
árvore. Eles exercitam o runtime geral, mas não fazem parte da implementação
de produção nem criam DLLs específicas.

Cada cenário deve declarar o aplicativo, a ação segura, o critério de término,
os limites externos e a evidência ON/OFF quando aplicável.

O `7zip/7z_cli_smoke.cpp` é executado pelo teste `seven_zip_cli_smoke` com
`7z_x64.exe` e pelo teste `seven_zip_subdir_cli_smoke` com `7-Zip/7z.exe`. O cenário
cobre arquivos `stored`, `DEFLATE`, `7z/LZMA2` normal e protegido por senha,
sobrescrita controlada com `-aoa`, rejeição de senha incorreta sem payload válido,
caminhos relativos e o filtro stdin/stdout (`-si`/`-so`).
O CMake registra o teste automaticamente quando `TL_POPULAR_APPS_DIR` aponta
para `Aplicativos_Windows_Populares/`.

Os testes `notepadpp_fh4_headless_smoke`, `notepadpp_root_fh4_headless_smoke` e
`notepadpp_extracted_fh4_headless_smoke` validam o despacho C++ EH FH4 sem X11
para `Notepad++/notepad++.exe`, `notepad++.exe` da raiz e `npp_extracted/notepad++.exe`.

O mesmo parâmetro registra `popular_apps_report_matrix`, que analisa os 26 PE
selecionados e o MSIX do corpus e verifica os exit codes esperados sem mapear
ou executar os arquivos. O caso é repetido no build Rust ON e no baseline C++
OFF.

`popular_apps_recursive_report_matrix` complementa essa seleção com todos os
64 PE/DLL/MSIX encontrados recursivamente no corpus atual, incluindo cópias
extraídas de 7-Zip e Notepad++. Ela continua sendo somente análise: DLLs,
pacotes e instaladores rejeitados nunca são iniciados, e cada exit esperado é
fixado no catálogo do teste para que uma mudança de formato ou arquitetura
falhe de forma visível.

`popular_apps_native_matrix` executa quatorze casos PE32+ com ação direta
ou rejeição pré-entry já documentados: os cinco binários do ecossistema 7-Zip
(`7z_x64.exe`, `7zFM_x64.exe`, `7-Zip/7z.exe`, `7-Zip/7zFM.exe`, `7-Zip/7zG.exe`),
os dois binários WinRAR, Rockstar, Rufus, HWiNFO64, Logitech G HUB e seu alias, e os
dois executáveis GUP de atualização do Notepad++ (`Notepad++/updater/GUP.exe` e `npp_extracted/updater/GUP.exe`).
Cada caso usa prefixo temporário, ambiente headless, `--timeout 3`, limites de CPU/memória e
verifica o exit code e o diagnóstico esperado; instaladores, DLLs e cenários
interativos de GUI permanecem nos testes específicos.

`popular_apps_install_matrix` cobre doze casos controlados: Roblox, G HUB, o
alias byte-a-byte, o preparo de HWiNFO e as rejeições pré-extração por arquitetura
x86 de CPU-Z, GPU-Z, HWMonitor, CapCut, Creative Cloud, Epic Installer, Office
Deployment Tool e RTSS. Cada caso usa `HOME`, `XDG_CONFIG_HOME`, `APPDATA` e
prefixo temporários, verifica o estágio/erro esperado e exige ausência de
arquivos residuais, extração e cadastro após a rejeição. O pacote MSIX de grande porte
Affinity x64 é instalado e registrado com sucesso e validado em testes próprios.
Instaladores PE32/x86 permanecem fora da execução funcional direta.

`popular_apps_archive_install_matrix` cobre os instaladores de arquivo auto-extraíveis
NSIS/SFX do corpus (`7-Zip_x64_Installer.exe`, `Notepad++_x64_Installer.exe`,
`Everything_Search_x64.exe` e `RTSSSetup737.exe`), verificando a extração do payload
PE32+ (x86-64) em prefixo isolado, o cadastro bem-sucedido no catálogo (`tradutorlinux app list`),
a execução do binário CLI quando aplicável (`7z.exe` com exit `0` e `everything.exe` com
`-create-file-list` indexando a árvore de diretórios e gerando arquivo `.efu` válido com exit `0`)
e a posterior remoção limpa da aplicação do catálogo (`tradutorlinux app remove <id>`).

Com o mesmo parâmetro e Xvfb disponível, os smokes GUI reais de 7-Zip File
Manager (`seven_zip_gui_smoke` e `seven_zip_subdir_gui_smoke`), PuTTY (`putty_real_gui_smoke`
e `putty_ssh_local_probe`), WinRAR (`winrar_sfx_real_smoke`, `winrar_extract_real_smoke`,
`winrar_723_sfx_real_smoke` e `winrar_723_extract_real_smoke`) e Notepad++
(`notepadpp_real_gui_smoke`) também são registrados separadamente no CTest.
O smoke do 7-Zip prepara catálogo, prefixo e `compat/profile.json` schema 4
temporários, cadastra o aplicativo com ID `7zip` e executa somente por `app run`; o
trace exige `compat-profile` carregado e `compat-extension` selecionada. Cada cenário
cria seu próprio Xvfb/staging e mantém seu resultado limitado ao comportamento documentado
na matriz de compatibilidade.
