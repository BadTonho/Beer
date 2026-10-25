if(NOT DEFINED TL_RUNTIME OR NOT DEFINED TL_CORPUS OR NOT DEFINED TL_STAGING_ROOT)
    message(FATAL_ERROR "TL_RUNTIME, TL_CORPUS e TL_STAGING_ROOT são obrigatórios")
endif()
if(NOT EXISTS "${TL_RUNTIME}" OR NOT IS_DIRECTORY "${TL_CORPUS}")
    message(FATAL_ERROR "runtime ou corpus inexistente")
endif()

# Os nomes x64 destes instaladores descrevem os payloads anunciados. Os
# executáveis-container presentes no corpus são PE32 x86, fora da arquitetura
# aceita pelo runtime. Verifique a rejeição sem extração ou cadastro parciais.
set(CASES
    "7-Zip_x64_Installer.exe|sevenzip|drive_c/Program Files/sevenzip/7z.exe"
    "Notepad++_x64_Installer.exe|notepadpp|drive_c/Program Files/notepadpp/notepad++.exe"
    "Everything_Search_x64.exe|everything|drive_c/Program Files/everything/$PLUGINSDIR/Everything/everything.exe"
    "RTSSSetup737.exe|rtss|drive_c/Program Files/rtss/DesktopOverlayHost64.exe"
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
    set(input_path "${TL_CORPUS}/${relative_path}")
    if(NOT EXISTS "${input_path}")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR "caso ausente: ${relative_path}")
    endif()

    math(EXPR index "${index} + 1")
    set(case_root "${TL_STAGING_ROOT}/case-${index}")
    set(home "${case_root}/home")
    set(config "${case_root}/config")
    set(data "${case_root}/data")
    set(prefix "${case_root}/prefix")
    file(MAKE_DIRECTORY "${home}" "${config}" "${data}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
            "BEER_PREFIX=${case_root}/report-prefix"
            "${TL_RUNTIME}" --report "${input_path}"
        TIMEOUT 30
        RESULT_VARIABLE report_exit
        OUTPUT_VARIABLE report_stdout
        ERROR_VARIABLE report_stderr
    )
    string(FIND "${report_stderr}" "0x14c não suportada" report_architecture_error)
    if(NOT "${report_exit}" STREQUAL "5" OR
       report_architecture_error EQUAL -1 OR
       EXISTS "${case_root}/report-prefix")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: --report deveria rejeitar PE32 sem criar prefixo; "
            "exit=${report_exit}\nstdout:\n${report_stdout}\nstderr:\n${report_stderr}")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
            "${TL_RUNTIME}" install "${input_path}" --name "${app_id}"
                --prefix "${prefix}" --trace
        TIMEOUT 30
        RESULT_VARIABLE install_exit
        OUTPUT_VARIABLE install_stdout
        ERROR_VARIABLE install_trace
    )
    string(FIND "${install_trace}" "failed stage=\"parse\"" install_parse_failure)
    string(FIND "${install_trace}" "status=\"unsupported-architecture\""
           install_architecture_error)
    string(FIND "${install_trace}" "extracted" unexpected_extraction)
    string(FIND "${install_trace}" "registered" unexpected_registration)
    if(NOT "${install_exit}" STREQUAL "5" OR
       install_parse_failure EQUAL -1 OR
       install_architecture_error EQUAL -1 OR
       NOT unexpected_extraction EQUAL -1 OR
       NOT unexpected_registration EQUAL -1 OR
       EXISTS "${prefix}/${expected_payload_relpath}")
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: install deveria rejeitar PE32 sem extrair payload; "
            "exit=${install_exit}\nstdout:\n${install_stdout}\nstderr:\n${install_trace}")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
            "${TL_RUNTIME}" app list
        TIMEOUT 10
        RESULT_VARIABLE list_exit
        OUTPUT_VARIABLE list_stdout
        ERROR_VARIABLE list_stderr
    )
    string(FIND "${list_stdout}" "[${app_id}]" catalog_position)
    if(NOT "${list_exit}" STREQUAL "0" OR NOT catalog_position EQUAL -1)
        file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
        message(FATAL_ERROR
            "${relative_path}: app list falhou ou há cadastro após rejeição; "
            "exit=${list_exit}\nstdout:\n${list_stdout}\nstderr:\n${list_stderr}")
    endif()
    math(EXPR passed "${passed} + 1")
endforeach()

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
message(STATUS
    "matriz de containers PE32 do corpus: ${passed}/${passed} rejeições confirmadas; "
    "nenhum payload extraído ou cadastrado")
