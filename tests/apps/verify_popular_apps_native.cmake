if(NOT DEFINED TL_RUNTIME OR NOT DEFINED TL_CORPUS OR NOT DEFINED TL_STAGING_ROOT)
    message(FATAL_ERROR "TL_RUNTIME, TL_CORPUS e TL_STAGING_ROOT são obrigatórios")
endif()

if(NOT EXISTS "${TL_RUNTIME}")
    message(FATAL_ERROR "runtime inexistente: ${TL_RUNTIME}")
endif()
if(NOT IS_DIRECTORY "${TL_CORPUS}")
    message(FATAL_ERROR "corpus inexistente: ${TL_CORPUS}")
endif()

# A matriz nativa não interage com janelas. Não herdar a sessão gráfica do
# desenvolvedor evita que um SFX GUI permaneça aguardando ação humana e torne o
# resultado dependente do ambiente que executou o CTest.
set(ENV{DISPLAY})
set(ENV{WAYLAND_DISPLAY})

# Estes são somente os PE32+ com uma ação direta segura já documentada no B2,
# mais rejeições controladas que devem parar antes do entry point. Instaladores,
# DLLs e cenários que exigem interação GUI têm smokes próprios e não são
# iniciados por esta matriz.
set(CASES
    "7z_x64.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "7zFM_x64.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "7-Zip/7z.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "7-Zip/7zFM.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "7-Zip/7zG.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "WinRAR_x64.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "winrar-x64-723.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "Rockstar-Games-Launcher.exe|1|exit exit-code=\"1\" explicit=\"sim\""
    "Rufus_x64.exe|4|entry point fora de uma página executável"
    "HWiNFO64.exe|4|entry point fora de uma página executável"
    "Logitech_GHUB_x64.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "lghub_installer.exe|0|exit exit-code=\"0\" explicit=\"sim\""
    "Notepad++/updater/GUP.exe|255|exit exit-code=\"4294967295\" explicit=\"sim\""
    "npp_extracted/updater/GUP.exe|255|exit exit-code=\"4294967295\" explicit=\"sim\""
)

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
file(MAKE_DIRECTORY "${TL_STAGING_ROOT}")
set(passed 0)
set(index 0)

foreach(case IN LISTS CASES)
    string(REPLACE "|" ";" fields "${case}")
    list(GET fields 0 relative_path)
    list(GET fields 1 expected_exit)
    list(GET fields 2 expected_marker)
    set(input_path "${TL_CORPUS}/${relative_path}")
    if(NOT EXISTS "${input_path}")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR "caso ausente: ${relative_path}")
    endif()

    math(EXPR index "${index} + 1")
    set(case_root "${TL_STAGING_ROOT}/case-${index}")
    set(prefix "${case_root}/prefix")
    set(home "${case_root}/home")
    set(config "${case_root}/config")
    set(data "${case_root}/data")
    set(appdata "${case_root}/appdata")
    file(MAKE_DIRECTORY "${prefix}" "${home}" "${config}" "${data}" "${appdata}")
    set(ENV{HOME} "${home}")
    set(ENV{XDG_CONFIG_HOME} "${config}")
    set(ENV{XDG_DATA_HOME} "${data}")
    set(ENV{TL_PREFIX} "${prefix}")
    set(ENV{APPDATA} "${appdata}")

    set(guest_timeout 3)
    set(cpu_limit 3)
    set(memory_limit 512)
    set(process_timeout 30)
    if("${relative_path}" STREQUAL "Logitech_GHUB_x64.exe" OR
       "${relative_path}" STREQUAL "lghub_installer.exe")
        # O bootstrapper consome cerca de 9 s no host antes do ExitProcess(0).
        # O limite antigo de 3 s encerrava o teste antes do resultado observável.
        set(guest_timeout 30)
        set(cpu_limit 30)
        set(memory_limit 1024)
        set(process_timeout 60)
    endif()

    execute_process(
        COMMAND "${TL_RUNTIME}" --trace=loader,process
            --timeout "${guest_timeout}" --cpu "${cpu_limit}" --memory "${memory_limit}"
            "${input_path}"
        TIMEOUT "${process_timeout}"
        RESULT_VARIABLE actual_exit
        OUTPUT_VARIABLE run_stdout
        ERROR_VARIABLE run_stderr
    )
    if(NOT "${actual_exit}" STREQUAL "${expected_exit}")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: exit esperado ${expected_exit}, obtido ${actual_exit}\n"
            "stdout:\n${run_stdout}\n"
            "stderr:\n${run_stderr}")
    endif()
    string(FIND "${run_stderr}" "${expected_marker}" marker_position)
    if(marker_position EQUAL -1)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: diagnóstico esperado não encontrado: ${expected_marker}\n"
            "stdout:\n${run_stdout}\n"
            "stderr:\n${run_stderr}")
    endif()
    if("${relative_path}" STREQUAL "Notepad++/updater/GUP.exe" OR
       "${relative_path}" STREQUAL "npp_extracted/updater/GUP.exe")
        string(FIND "${run_stderr}" "unmap base=\"" unmap_position)
        string(FIND "${run_stderr}" "ExitProcess symbol=\"" exit_process_position)
        string(FIND "${run_stderr}" "guest-signal" guest_signal_position)
        string(FIND "${run_stderr}" "guest-timeout" guest_timeout_position)
        if(unmap_position EQUAL -1 OR NOT exit_process_position EQUAL -1 OR
           NOT guest_signal_position EQUAL -1 OR NOT guest_timeout_position EQUAL -1)
            file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
            message(FATAL_ERROR
                "${relative_path}: rejeição não terminou antes do entry point:\n"
                "stdout:\n${run_stdout}\n"
                "stderr:\n${run_stderr}")
        endif()
    endif()
    math(EXPR passed "${passed} + 1")
endforeach()

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
message(STATUS "matriz nativa do corpus: ${passed}/${passed} casos passaram")
