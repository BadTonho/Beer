cmake_minimum_required(VERSION 3.28)

project(Beer VERSION 0.0.0 LANGUAGES C CXX ASM)

include(GNUInstallDirs)

include(CTest)
include(FetchContent)

option(TL_ENABLE_GUI "Build the Qt GUI launcher and its tests (requires Qt6)" ON)
if(TL_ENABLE_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()
find_package(ZLIB REQUIRED)

option(TL_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(TL_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(TL_BUILD_SAMPLES "Build PE32+ integration fixtures" ON)
option(TL_BUILD_TARGET_APPS "Cross-compile real target applications (downloads pinned sources)" OFF)
option(TL_BUILD_RUST "Build and use the opt-in Rust validators" OFF)
set(TL_RUST_TOOLCHAIN "1.97.1" CACHE STRING "Pinned rustup toolchain for the Rust validators")
set(TL_PROTON_ROOT "" CACHE PATH "Proton root for optional real backend integration tests")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Do not update fetched dependencies after the first download" FORCE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)

if(TL_BUILD_RUST)
    include(RustValidator)
    tl_configure_rust_validator()
endif()

set(TL_GUI_INSTALL_TARGET "")
if(TL_ENABLE_GUI)
    list(APPEND TL_GUI_INSTALL_TARGET tradutorlinux_gui)
endif()

add_subdirectory(src)

install(TARGETS tradutorlinux ${TL_GUI_INSTALL_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES resources/beer.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES beer.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "beer")
set(CPACK_PACKAGE_VENDOR "Beer Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Runtime de compatibilidade Win32 para Linux")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Beer Project")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libx11-6, libwayland-client0, libxkbcommon0, libqt6widgets6, zlib1g, libstdc++6, libgcc-s1, desktop-file-utils")
set(CPACK_RPM_PACKAGE_LICENSE "LGPL-3.0-only")
set(CPACK_RPM_PACKAGE_AUTOREQ "ON")
set(CPACK_RPM_PACKAGE_REQUIRES "desktop-file-utils")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

if(BUILD_TESTING)
    set(INSTALL_GTEST OFF CACHE BOOL "Disable installation of GoogleTest" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "Do not build GoogleMock" FORCE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 063de7e9578f82b369302001269680b4b1553359
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()

include(CPack)
