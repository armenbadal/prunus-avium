find_package(Catch2 3 REQUIRED)

add_executable(${PROJECT_NAME}-tests
    ast_test.cxx
    astlisp_test.cxx
    codegensi_test.cxx
    formatters_test.cxx
    lexeme_test.cxx
    parser_test.cxx
    scanner_test.cxx
    semantic_test.cxx
    symbols_test.cxx
    "${PROJECT_SOURCE_DIR}/src/astlisp.cxx"
    "${PROJECT_SOURCE_DIR}/src/codegensi.cxx"
    "${PROJECT_SOURCE_DIR}/src/diagnostics.cxx"
    "${PROJECT_SOURCE_DIR}/src/lexeme.cxx"
    "${PROJECT_SOURCE_DIR}/src/parser.cxx"
    "${PROJECT_SOURCE_DIR}/src/scanner.cxx"
    "${PROJECT_SOURCE_DIR}/src/semantic.cxx"
    "${PROJECT_SOURCE_DIR}/src/symbols.cxx")
target_compile_features(${PROJECT_NAME}-tests PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME}-tests PRIVATE
    AVIUM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME}-tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME}-tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME ${PROJECT_NAME}.tests COMMAND ${PROJECT_NAME}-tests)

set(codegen_e2e_directory "${CMAKE_CURRENT_BINARY_DIR}/codegen-e2e")
set(codegen_e2e_input "${codegen_e2e_directory}/case05.bas")
set(codegen_e2e_source "${codegen_e2e_directory}/case05.c")
file(MAKE_DIRECTORY "${codegen_e2e_directory}")

add_custom_command(
    OUTPUT "${codegen_e2e_source}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/examples/case05.bas"
        "${codegen_e2e_input}"
    COMMAND $<TARGET_FILE:prunus> "${codegen_e2e_input}"
    DEPENDS
        prunus
        "${PROJECT_SOURCE_DIR}/examples/case05.bas"
    WORKING_DIRECTORY "${codegen_e2e_directory}"
    COMMENT "Generating C code for the end-to-end test"
    VERBATIM)

add_executable(${PROJECT_NAME}-codegen-e2e "${codegen_e2e_source}")
target_link_libraries(${PROJECT_NAME}-codegen-e2e PRIVATE avium::runtime)

add_test(NAME ${PROJECT_NAME}.codegen.e2e COMMAND ${PROJECT_NAME}-codegen-e2e)

set(codegen_return_directory "${CMAKE_CURRENT_BINARY_DIR}/codegen-return")
set(codegen_return_input "${codegen_return_directory}/return.bas")
set(codegen_return_source "${codegen_return_directory}/return.c")
file(MAKE_DIRECTORY "${codegen_return_directory}")

add_custom_command(
    OUTPUT "${codegen_return_source}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/return.bas"
        "${codegen_return_input}"
    COMMAND $<TARGET_FILE:prunus> "${codegen_return_input}"
    DEPENDS
        prunus
        "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/return.bas"
    WORKING_DIRECTORY "${codegen_return_directory}"
    COMMENT "Generating C code for the return end-to-end test"
    VERBATIM)

add_executable(${PROJECT_NAME}-codegen-return "${codegen_return_source}")
target_link_libraries(${PROJECT_NAME}-codegen-return PRIVATE avium::runtime)

add_test(NAME ${PROJECT_NAME}.codegen.return COMMAND ${PROJECT_NAME}-codegen-return)
