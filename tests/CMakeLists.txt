find_package(Catch2 3 REQUIRED)

add_executable(${PROJECT_NAME}-tests
    ast_test.cxx
    astlisp_test.cxx
    lexeme_test.cxx
    parser_test.cxx
    scanner_test.cxx
    "${PROJECT_SOURCE_DIR}/src/astlisp.cxx"
    "${PROJECT_SOURCE_DIR}/src/diagnostics.cxx"
    "${PROJECT_SOURCE_DIR}/src/lexeme.cxx"
    "${PROJECT_SOURCE_DIR}/src/parser.cxx"
    "${PROJECT_SOURCE_DIR}/src/scanner.cxx")
target_compile_features(${PROJECT_NAME}-tests PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME}-tests PRIVATE
    AVIUM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME}-tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME}-tests PRIVATE Catch2::Catch2WithMain)

add_test(NAME ${PROJECT_NAME}.tests COMMAND ${PROJECT_NAME}-tests)
