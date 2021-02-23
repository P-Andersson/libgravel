add_executable(gravel_dynamic_value_example)
target_sources(gravel_dynamic_value_example
               PRIVATE
                  src/dynamic_value.cpp)
target_link_libraries(gravel_dynamic_value_example PUBLIC gravel)

