if(NOT DEFINED TL_RUNTIME OR NOT DEFINED TL_CORPUS OR NOT DEFINED TL_STAGING_ROOT)
    message(FATAL_ERROR "TL_RUNTIME, TL_CORPUS e TL_STAGING_ROOT são obrigatórios")
endif()

if(NOT EXISTS "${TL_RUNTIME}")
    message(FATAL_ERROR "runtime inexistente: ${TL_RUNTIME}")
endif()
if(NOT IS_DIRECTORY "${TL_CORPUS}")
    message(FATAL_ERROR "corpus inexistente: ${TL_CORPUS}")
endif()

# Não herdar display gráfico
set(ENV{DISPLAY})
set(ENV{WAYLAND_DISPLAY})

# Casos de instaladores NSIS/archive com payloads PE32+ (x86-64) internos:
# relative_path | app_id | expected_payload_relpath | can_run_smoke
set(CASES
    "7-Zip_x64_Installer.exe|sevenzip|drive_c/Program Files/sevenzip/7z.exe|1"
    "Notepad++_x64_Installer.exe|notepadpp|drive_c/Program Files/notepadpp/notepad++.exe|0"
    "Everything_Search_x64.exe|everything|drive_c/Program Files/everything/$PLUGINSDIR/Everything/everything.exe|everything_file_list"
    "RTSSSetup737.exe|rtss|drive_c/Program Files/rtss/DesktopOverlayHost64.exe|0"
)

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
file(MAKE_DIRECTORY "${TL_STAGING_ROOT}")
set(passed 0)
set(index 0)

foreach(case IN LISTS CASES)
    string(REPLACE "|" ";" fields "${case}")
    list(GET fields 0 relative_path)
    list(GET fields 1 app_id)
    list(GET fields 2 expected_payload_relpath)
    list(GET fields 3 can_run_smoke)

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
    set(appdata "${case_root}/appdata")
    file(MAKE_DIRECTORY "${prefix}" "${home}" "${config}" "${appdata}")
    set(ENV{HOME} "${home}")
    set(ENV{XDG_CONFIG_HOME} "${config}")
    set(ENV{APPDATA} "${appdata}")

    # 1. Executar instalação e extração
    execute_process(
        COMMAND "${TL_RUNTIME}" install "${input_path}" --name "${app_id}"
            --prefix "${prefix}" --timeout 15 --trace
        TIMEOUT 45
        RESULT_VARIABLE install_exit
        OUTPUT_VARIABLE install_stdout
        ERROR_VARIABLE install_stderr
    )
    if(NOT "${install_exit}" EQUAL 0)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: install falhou com código ${install_exit}\n"
            "stdout:\n${install_stdout}\n"
            "stderr:\n${install_stderr}")
    endif()

    # 2. Verificar que o executável esperado foi extraído
    set(payload_path "${prefix}/${expected_payload_relpath}")
    if(NOT EXISTS "${payload_path}")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: executável esperado não encontrado no prefixo: ${payload_path}")
    endif()

    # 3. Verificar listagem no catálogo
    execute_process(
        COMMAND "${TL_RUNTIME}" app list
        TIMEOUT 10
        RESULT_VARIABLE list_exit
        OUTPUT_VARIABLE list_stdout
        ERROR_VARIABLE list_stderr
    )
    if(NOT "${list_exit}" EQUAL 0)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR "${relative_path}: app list falhou:\n${list_stderr}")
    endif()
    string(FIND "${list_stdout}" "[${app_id}]" id_pos)
    if(id_pos EQUAL -1)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: app_id [${app_id}] não encontrado na listagem:\n${list_stdout}")
    endif()

    # 4. Executar smoke CLI do aplicativo quando suportado (ex: 7z.exe, everything.exe)
    if("${can_run_smoke}" STREQUAL "1")
        execute_process(
            COMMAND "${TL_RUNTIME}" app run "${app_id}" --timeout 5
            TIMEOUT 15
            RESULT_VARIABLE run_exit
            OUTPUT_VARIABLE run_stdout
            ERROR_VARIABLE run_stderr
        )
        if(NOT "${run_exit}" EQUAL 0)
            file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
            message(FATAL_ERROR
                "${relative_path}: app run ${app_id} falhou com código ${run_exit}\n"
                "stdout:\n${run_stdout}\n"
                "stderr:\n${run_stderr}")
        endif()
    elseif("${can_run_smoke}" STREQUAL "everything_file_list")
        set(efu_target "${prefix}/drive_c/test_filelist.efu")
        execute_process(
            COMMAND "${TL_RUNTIME}" app run "${app_id}" --timeout 10 -create-file-list "C:\\test_filelist.efu" "C:\\Program Files"
            TIMEOUT 20
            RESULT_VARIABLE run_exit
            OUTPUT_VARIABLE run_stdout
            ERROR_VARIABLE run_stderr
        )
        if(NOT "${run_exit}" EQUAL 0)
            file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
            message(FATAL_ERROR
                "${relative_path}: app run ${app_id} -create-file-list falhou com código ${run_exit}\n"
                "stdout:\n${run_stdout}\n"
                "stderr:\n${run_stderr}")
        endif()
        if(NOT EXISTS "${efu_target}")
            file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
            message(FATAL_ERROR
                "${relative_path}: arquivo .efu não foi gerado pelo app run: ${efu_target}")
        endif()
    endif()

    # 5. Remover do catálogo
    execute_process(
        COMMAND "${TL_RUNTIME}" app remove "${app_id}"
        TIMEOUT 10
        RESULT_VARIABLE remove_exit
        OUTPUT_VARIABLE remove_stdout
        ERROR_VARIABLE remove_stderr
    )
    if(NOT "${remove_exit}" EQUAL 0)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: app remove ${app_id} falhou com código ${remove_exit}\n"
            "stdout:\n${remove_stdout}\n"
            "stderr:\n${remove_stderr}")
    endif()

    # 6. Confirmar que não consta mais no catálogo
    execute_process(
        COMMAND "${TL_RUNTIME}" app list
        TIMEOUT 10
        RESULT_VARIABLE list2_exit
        OUTPUT_VARIABLE list2_stdout
        ERROR_VARIABLE list2_stderr
    )
    string(FIND "${list2_stdout}" "[${app_id}]" id2_pos)
    if(NOT id2_pos EQUAL -1)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: app_id [${app_id}] ainda consta no catálogo após remoção:\n${list2_stdout}")
    endif()

    math(EXPR passed "${passed} + 1")
endforeach()

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
message(STATUS "matriz de instaladores compactados do corpus: ${passed}/${passed} casos passaram")
