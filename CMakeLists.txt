cmake_minimum_required(VERSION 4.2)

project(prunus VERSION 0.0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    src/main.cxx
    src/lexeme.cxx
    src/scanner.cxx
    src/diagnostics.cxx
    src/parser.cxx
    src/symbols.cxx
    src/semantic.cxx
    src/astlisp.cxx)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

include(CTest)

add_subdirectory(tests)
