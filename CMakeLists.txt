cmake_minimum_required(VERSION 4.2)

project(prunus VERSION 0.0.1 LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

add_executable(${PROJECT_NAME}
    src/main.cxx
    src/lexeme.cxx
    src/scanner.cxx
    src/diagnostics.cxx
    src/parser.cxx
    src/symbols.cxx
    src/semantic.cxx
    src/codegen.cxx
    src/astlisp.cxx)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE ${LLVM_DEFINITIONS_LIST})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE LLVMCore)

include(CTest)

add_subdirectory(tests)
