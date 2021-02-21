add_library(gravel INTERFACE)

target_include_directories(gravel INTERFACE include)

find_package(Catch2)
if (Catch2_FOUND)
	add_subdirectory(tests)
endif()