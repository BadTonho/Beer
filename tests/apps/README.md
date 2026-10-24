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
Cada caso usa `HOME`, configuração e dados XDG, `APPDATA` e prefixo isolados em
ambiente headless, e verifica o exit code e o diagnóstico esperado. G HUB recebe
limites de 30 s para execução/CPU e 1 GiB porque seu bootstrapper precisa de
cerca de 9 s para chegar a `ExitProcess(0)` neste host; os demais casos mantêm
3 s e 512 MiB. Instaladores, DLLs e cenários interativos de GUI permanecem nos
testes específicos.

`popular_apps_install_matrix` cobre doze casos controlados: Roblox, G HUB, o
alias byte-a-byte, o preparo de HWiNFO e as rejeições pré-extração por arquitetura
x86 de CPU-Z, GPU-Z, HWMonitor, CapCut, Creative Cloud, Epic Installer, Office
Deployment Tool e RTSS. Cada caso usa `HOME`, `XDG_CONFIG_HOME`, `APPDATA` e
prefixo temporários, verifica o estágio/erro esperado e exige ausência de
arquivos residuais, extração e cadastro após a rejeição. O pacote MSIX de grande porte
Affinity x64 tem o smoke `affinity_msix_install_smoke`: análise do MSIX,
extração e cadastro, análise do executável principal, remoção do cadastro e
preservação dos arquivos. O relatório identifica .NET CLR; a aplicação não é
iniciada pelo runtime nativo.
Instaladores PE32/x86 permanecem fora da execução funcional direta.

`popular_apps_x86_installer_rejection_matrix` verifica `--report` e tentativa de
instalação dos quatro containers com nome x64 no corpus (`7-Zip_x64_Installer.exe`,
`Notepad++_x64_Installer.exe`, `Everything_Search_x64.exe` e `RTSSSetup737.exe`).
Todos são PE32 (x86): o teste exige exit `5`, diagnóstico `unsupported-architecture`,
nenhum executável extraído e nenhum cadastro. Os smokes dos executáveis PE32+
disponíveis do 7-Zip e do Notepad++ são independentes; o corpus atual não inclui
um `everything.exe` autônomo para repetir o fluxo `.efu` nesta etapa.

Com o mesmo parâmetro e Xvfb disponível, os smokes GUI reais de 7-Zip File
Manager (`seven_zip_gui_smoke` e `seven_zip_subdir_gui_smoke`), PuTTY (`putty_real_gui_smoke`
e `putty_ssh_local_probe`), WinRAR (`winrar_sfx_real_smoke`, `winrar_extract_real_smoke`,
`winrar_723_sfx_real_smoke` e `winrar_723_extract_real_smoke`), Notepad++
(`notepadpp_real_gui_smoke`) e Rockstar Games Launcher (`rockstar_real_gui_smoke`)
também são registrados separadamente no CTest.
O smoke do 7-Zip prepara catálogo, prefixo e `compat/profile.json` schema 4
temporários, cadastra o aplicativo com ID `7zip` e executa somente por `app run`; o
trace exige `compat-profile` carregado e `compat-extension` selecionada. O smoke do
7-Zip verifica o arquivo de saída da cópia e o exit code `0`. Os smokes
`winrar_extract_real_smoke` e `winrar_723_extract_real_smoke` mantêm a extração do
instalador SFX como uma verificação separada e, depois, criam um `.rar` com o
`Rar.exe` extraído, cadastram temporariamente `UnRAR.exe`, extraem o arquivo por
`app run` e comparam byte a byte o conteúdo resultante. O cadastro temporário é
removido ao final; o staging e os prefixos são novos em cada execução. O smoke do
Rockstar Games Launcher valida a inicialização da janela GUI "Rockstar Games Launcher Installer"
sob Xvfb, recepção de eventos de encerramento via `WM_DELETE_WINDOW` e finalização limpa
com exit code 1. Cada cenário cria seu próprio Xvfb/staging e mantém seu resultado
limitado ao comportamento documentado na matriz de compatibilidade.
