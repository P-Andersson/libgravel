cmake_minimum_required(VERSION 3.18)
project(libgravel)

add_library(gravel INTERFACE)

target_include_directories(gravel INTERFACE include)


set(GRAVEL_BUILD_EXAMPLES NO CACHE BOOL "Controls if gravels examples should be built or not")
if (GRAVEL_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

find_package(Catch2)
if (Catch2_FOUND)
	add_subdirectory(tests)
endif()