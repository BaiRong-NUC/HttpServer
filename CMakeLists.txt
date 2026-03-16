cmake_minimum_required(VERSION 3.15)

project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_APP "Build App Executables" ON)
option(BUILD_API_TEST "Build Api Test Executables" ON)

file(GLOB SRC_CPP_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(httpserver STATIC ${SRC_CPP_FILES})
add_library(HttpServer::httpserver ALIAS httpserver)

target_compile_features(httpserver PUBLIC cxx_std_17)
target_include_directories(httpserver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(httpserver PUBLIC Threads::Threads)

set_target_properties(httpserver PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
	OUTPUT_NAME httpserver
)

install(TARGETS httpserver
	EXPORT HttpServerTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(BUILD_APP)
	file(GLOB APP_CPP_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp)

	foreach(APP_CPP_FILE ${APP_CPP_FILES})
		get_filename_component(APP_TARGET_NAME ${APP_CPP_FILE} NAME_WE)
		add_executable(${APP_TARGET_NAME} ${APP_CPP_FILE})
		target_link_libraries(${APP_TARGET_NAME} PRIVATE HttpServer::httpserver)
		set_target_properties(${APP_TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/app
		)
	endforeach()
endif()

# 编译开发阶段的 API 测试代码
if(BUILD_API_TEST)
	set(API_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api_test)

	file(GLOB_RECURSE API_CPP_FILES CONFIGURE_DEPENDS ${API_TEST_DIR}/*.cpp)

	set(API_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/api_test)

	foreach(API_CPP_FILE ${API_CPP_FILES})
		get_filename_component(FILE_NAME ${API_CPP_FILE} NAME_WE)
		file(RELATIVE_PATH REL_PATH ${API_TEST_DIR} ${API_CPP_FILE})
		get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)
		string(REPLACE "/" "_" TARGET_SUFFIX ${REL_PATH})
		string(REPLACE ".cpp" "" TARGET_SUFFIX ${TARGET_SUFFIX})
		set(TARGET_NAME api_test_${TARGET_SUFFIX})
		string(FIND ${REL_PATH} "api/" IS_API_SUBDIR)

		if(IS_API_SUBDIR EQUAL 0)
			add_executable(${TARGET_NAME} ${API_CPP_FILE})
			target_link_libraries(${TARGET_NAME} PRIVATE HttpServer::httpserver)
		else()
			add_executable(${TARGET_NAME} ${API_CPP_FILE})
			target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)
		endif()

		if(REL_DIR STREQUAL "")
			set(TARGET_OUTPUT_DIR ${API_EXECUTABLE_OUTPUT_PATH})
		else()
			set(TARGET_OUTPUT_DIR ${API_EXECUTABLE_OUTPUT_PATH}/${REL_DIR})
		endif()

		set_target_properties(${TARGET_NAME} PROPERTIES
			OUTPUT_NAME ${FILE_NAME}
			RUNTIME_OUTPUT_DIRECTORY ${TARGET_OUTPUT_DIR}
		)
	endforeach()
endif()