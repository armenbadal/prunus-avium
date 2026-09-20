add_executable(avium-runtime-tests abi_test.c)
target_link_libraries(avium-runtime-tests PRIVATE avium::runtime)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(avium-runtime-tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror)
elseif(MSVC)
    target_compile_options(avium-runtime-tests PRIVATE /W4 /WX)
endif()

add_test(NAME avium.runtime.abi COMMAND avium-runtime-tests)
