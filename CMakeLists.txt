cmake_minimum_required(VERSION 3.15)

project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_API_TEST "Build Api Test Executables" ON)

if(BUILD_API_TEST)
	add_executable(time_wheel api_test/time/time_wheel.cpp)
	set_target_properties(time_wheel PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/api_test
	)
endif()