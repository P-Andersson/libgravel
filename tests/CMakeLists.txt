add_executable(gravel_tests)

target_include_directories(gravel_tests PUBLIC include PRIVATE src)

target_sources(gravel_tests
               PRIVATE
                  src/main.cpp
                  src/test_dynamic_value.cpp
                  src/test_unique_function.cpp)

find_package(Catch2)

target_link_libraries(gravel_tests PRIVATE gravel Catch2::Catch2)

add_test(NAME gravel_tests COMMAND gravel_tests)

add_custom_command(TARGET gravel_tests
                   POST_BUILD
                   COMMAND ctest gravel_tests -C $<CONFIGURATION> --output-on-failure)