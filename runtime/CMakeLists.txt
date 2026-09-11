cmake_minimum_required(VERSION 4.2)

project(avium-runtime LANGUAGES C)

include(CheckLibraryExists)
include(CTest)
include(GNUInstallDirs)

add_library(avium-runtime STATIC)
add_library(avium::runtime ALIAS avium-runtime)

target_sources(avium-runtime
    PRIVATE
        src/arrays.c
        src/errors.c
        src/errors.h
        src/io.c
        src/numbers.c
        src/texts.c
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            include/arrays.h
            include/io.h
            include/numbers.h
            include/texts.h)

set_target_properties(avium-runtime PROPERTIES
    C_EXTENSIONS OFF
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    EXPORT_NAME runtime)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(avium-runtime PRIVATE
        -Wall
        -Wextra
        -Wpedantic)
elseif(MSVC)
    target_compile_options(avium-runtime PRIVATE /W4)
endif()

check_library_exists(m sqrt "" AVIUM_RUNTIME_HAS_LIBM)
if(AVIUM_RUNTIME_HAS_LIBM)
    target_link_libraries(avium-runtime PUBLIC m)
endif()

if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

install(TARGETS avium-runtime
    EXPORT avium-runtime-targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT avium-runtime-targets
    FILE avium-runtime-targets.cmake
    NAMESPACE avium::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/avium-runtime")
