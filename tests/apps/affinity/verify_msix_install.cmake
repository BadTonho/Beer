if(NOT DEFINED TL_RUNTIME OR NOT DEFINED TL_PACKAGE OR NOT DEFINED TL_STAGING_ROOT)
    message(FATAL_ERROR "TL_RUNTIME, TL_PACKAGE e TL_STAGING_ROOT são obrigatórios")
endif()
if(NOT EXISTS "${TL_RUNTIME}" OR NOT EXISTS "${TL_PACKAGE}")
    message(FATAL_ERROR "runtime ou pacote MSIX não encontrado")
endif()

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
set(home "${TL_STAGING_ROOT}/home")
set(config "${TL_STAGING_ROOT}/config")
set(data "${TL_STAGING_ROOT}/data")
set(prefix "${TL_STAGING_ROOT}/prefix")
file(MAKE_DIRECTORY "${home}" "${config}" "${data}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "TL_PREFIX=${TL_STAGING_ROOT}/report-prefix"
        "${TL_RUNTIME}" --report "${TL_PACKAGE}"
    TIMEOUT 60
    RESULT_VARIABLE report_exit
    OUTPUT_VARIABLE package_report
    ERROR_VARIABLE report_trace
)
string(FIND "${package_report}" "package-name: Canva.Affinity" package_name_position)
string(FIND "${package_report}" "main-executable: App\\Affinity.exe" main_executable_position)
if(NOT "${report_exit}" STREQUAL "0" OR
   package_name_position EQUAL -1 OR main_executable_position EQUAL -1 OR
   EXISTS "${TL_STAGING_ROOT}/report-prefix")
    message(FATAL_ERROR
        "Affinity MSIX report inesperado; exit=${report_exit}\n"
        "stdout:\n${package_report}\nstderr:\n${report_trace}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "${TL_RUNTIME}" install "${TL_PACKAGE}" --name "Affinity"
        --prefix "${prefix}" --timeout 30 --trace --cpu 30 --memory 2048
    TIMEOUT 180
    RESULT_VARIABLE install_exit
    OUTPUT_VARIABLE install_stdout
    ERROR_VARIABLE install_trace
)
if(NOT "${install_exit}" STREQUAL "0")
    message(FATAL_ERROR
        "instalação MSIX do Affinity falhou; exit=${install_exit}\n"
        "stdout:\n${install_stdout}\nstderr:\n${install_trace}")
endif()
string(FIND "${install_trace}" "registered prefix=" registered_position)
if(registered_position EQUAL -1)
    message(FATAL_ERROR "trace do Affinity não confirmou o cadastro:\n${install_trace}")
endif()

set(executable "${prefix}/drive_c/Program Files/affinity/App/Affinity.exe")
if(NOT EXISTS "${executable}")
    message(FATAL_ERROR "executável principal não encontrado após extração: ${executable}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "${TL_RUNTIME}" app list
    TIMEOUT 15
    RESULT_VARIABLE list_exit
    OUTPUT_VARIABLE list_stdout
    ERROR_VARIABLE list_trace
)
string(FIND "${list_stdout}" "[affinity] Affinity" list_entry_position)
string(FIND "${list_stdout}" "${executable}" list_executable_position)
if(NOT "${list_exit}" STREQUAL "0" OR
   list_entry_position EQUAL -1 OR list_executable_position EQUAL -1)
    message(FATAL_ERROR
        "catálogo não contém o Affinity instalado; exit=${list_exit}\n"
        "stdout:\n${list_stdout}\nstderr:\n${list_trace}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "TL_PREFIX=${prefix}" "${TL_RUNTIME}" --report "${executable}"
    TIMEOUT 60
    RESULT_VARIABLE executable_report_exit
    OUTPUT_VARIABLE executable_report
    ERROR_VARIABLE executable_report_trace
)
string(FIND "${executable_report}" "runtime: .NET CLR (COM Descriptor header)" clr_position)
string(FIND "${executable_report}" "recommendation: aplicativo gerenciado .NET/CLR"
       dotnet_recommendation_position)
if(NOT "${executable_report_exit}" STREQUAL "0" OR
   clr_position EQUAL -1 OR dotnet_recommendation_position EQUAL -1)
    message(FATAL_ERROR
        "o relatório do executável Affinity não confirmou o limite .NET; "
        "exit=${executable_report_exit}\nstdout:\n${executable_report}\n"
        "stderr:\n${executable_report_trace}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "${TL_RUNTIME}" app remove affinity
    TIMEOUT 15
    RESULT_VARIABLE remove_exit
    OUTPUT_VARIABLE remove_stdout
    ERROR_VARIABLE remove_trace
)
string(FIND "${remove_stdout}" "removido" removed_position)
if(NOT "${remove_exit}" STREQUAL "0" OR removed_position EQUAL -1 OR
   NOT IS_DIRECTORY "${prefix}" OR NOT EXISTS "${executable}")
    message(FATAL_ERROR
        "remoção do cadastro do Affinity ou preservação dos arquivos falhou; "
        "exit=${remove_exit}\nstdout:\n${remove_stdout}\nstderr:\n${remove_trace}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "HOME=${home}" "XDG_CONFIG_HOME=${config}" "XDG_DATA_HOME=${data}"
        "${TL_RUNTIME}" app list
    TIMEOUT 15
    RESULT_VARIABLE final_list_exit
    OUTPUT_VARIABLE final_list_stdout
    ERROR_VARIABLE final_list_trace
)
string(FIND "${final_list_stdout}" "[affinity]" final_entry_position)
if(NOT "${final_list_exit}" STREQUAL "0" OR NOT final_entry_position EQUAL -1)
    message(FATAL_ERROR
        "catálogo ainda contém Affinity após a remoção; exit=${final_list_exit}\n"
        "stdout:\n${final_list_stdout}\nstderr:\n${final_list_trace}")
endif()

file(REMOVE_RECURSE "${TL_STAGING_ROOT}")
message(STATUS
    "Affinity MSIX: report=0, install=0, catálogo verificado, executável .NET analisado; "
    "startup não executado, cadastro removido e prefixo preservado")
