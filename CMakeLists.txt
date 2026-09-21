cmake_minimum_required(VERSION 4.2)

project(prunus VERSION 0.0.1 LANGUAGES C CXX)

include(CTest)

find_package(LLVM CONFIG REQUIRED)

separate_arguments(AVIUM_LLVM_DEFINITIONS NATIVE_COMMAND
    "${LLVM_DEFINITIONS}")

add_subdirectory(runtime)

add_executable(${PROJECT_NAME}
    src/main.cxx
    src/lexeme.cxx
    src/scanner.cxx
    src/diagnostics.cxx
    src/parser.cxx
    src/symbols.cxx
    src/semantic.cxx
    src/astlisp.cxx
    src/ircodegen.cxx
    src/runtimeabi.cxx)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${LLVM_INCLUDE_DIRS}")
target_compile_options(${PROJECT_NAME} PRIVATE
    ${AVIUM_LLVM_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    LLVMCore
    LLVMSupport)

add_subdirectory(tests)
