add_executable(gravel_dynamic_value_example)
target_sources(gravel_dynamic_value_example
               PRIVATE
                  src/dynamic_value.cpp)
target_link_libraries(gravel_dynamic_value_example PUBLIC gravel)

add_executable(gravel_unique_function_example)
target_sources(gravel_unique_function_example
               PRIVATE
                  src/unique_function.cpp)
target_link_libraries(gravel_unique_function_example PUBLIC gravel)
