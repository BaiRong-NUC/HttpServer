cmake_minimum_required(VERSION 3.15)

project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_APP "Build App Executables" ON)
option(BUILD_API_TEST "Build Api Test Executables" OFF)
option(BUILD_TEST "Build Test Executables" ON)

file(GLOB_RECURSE SRC_CPP_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(httpserver STATIC ${SRC_CPP_FILES})
add_library(HttpServer::httpserver ALIAS httpserver)

target_compile_features(httpserver PUBLIC cxx_std_17)
target_include_directories(httpserver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(httpserver PUBLIC Threads::Threads)

# Optional: libcurl for app HTTP client calls (used by app/server.cpp)
find_package(CURL REQUIRED)
if(CURL_FOUND)
	message(STATUS "Found libcurl: ${CURL_VERSION}")
endif()

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
	file(GLOB APP_SRC_CPP_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/app/src/*.cpp)
	file(GLOB_RECURSE APP_WWWROOT_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/app/wwwroot/*)

	add_custom_target(sync_app_wwwroot ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/app/wwwroot
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/app/wwwroot ${CMAKE_BINARY_DIR}/app/wwwroot
		DEPENDS ${APP_WWWROOT_FILES}
		COMMENT "Sync app static assets to build/app/wwwroot"
	)

	foreach(APP_CPP_FILE ${APP_CPP_FILES})
		get_filename_component(APP_TARGET_NAME ${APP_CPP_FILE} NAME_WE)
		add_executable(${APP_TARGET_NAME} ${APP_CPP_FILE} ${APP_SRC_CPP_FILES})
			target_link_libraries(${APP_TARGET_NAME} PRIVATE HttpServer::httpserver CURL::libcurl)
		target_include_directories(${APP_TARGET_NAME}
			PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/app/include
		)
		set_target_properties(${APP_TARGET_NAME} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/app
		)
		add_dependencies(${APP_TARGET_NAME} sync_app_wwwroot)
	endforeach()
endif()

# 编译 test 目录下的程序，输出到 build/test
if(BUILD_TEST)
	set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

	file(GLOB_RECURSE TEST_CPP_FILES CONFIGURE_DEPENDS ${TEST_DIR}/*.cpp)

	set(TEST_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test)

	foreach(TEST_CPP_FILE ${TEST_CPP_FILES})
		get_filename_component(FILE_NAME ${TEST_CPP_FILE} NAME_WE)
		file(RELATIVE_PATH REL_PATH ${TEST_DIR} ${TEST_CPP_FILE})
		get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)
		string(REPLACE "/" "_" TARGET_SUFFIX ${REL_PATH})
		string(REPLACE ".cpp" "" TARGET_SUFFIX ${TARGET_SUFFIX})
		set(TARGET_NAME test_${TARGET_SUFFIX})

		add_executable(${TARGET_NAME} ${TEST_CPP_FILE})
		target_link_libraries(${TARGET_NAME} PRIVATE HttpServer::httpserver)

		if(REL_DIR STREQUAL "")
			set(TARGET_OUTPUT_DIR ${TEST_EXECUTABLE_OUTPUT_PATH})
		else()
			set(TARGET_OUTPUT_DIR ${TEST_EXECUTABLE_OUTPUT_PATH}/${REL_DIR})
		endif()

		set_target_properties(${TARGET_NAME} PROPERTIES
			OUTPUT_NAME ${FILE_NAME}
			RUNTIME_OUTPUT_DIRECTORY ${TARGET_OUTPUT_DIR}
		)
	endforeach()
endif()

# 将运行脚本复制到构建目录的 app 下，方便在 build/app 运行脚本来启动服务并把日志写入该目录
if(BUILD_APP)
	# 在配置阶段将 loop.sh 复制到 build/app，并设置可执行权限(避免并行构建时的 cmake -E 竞态)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/app)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/app/loop.sh ${CMAKE_BINARY_DIR}/app/loop.sh COPYONLY)
	execute_process(COMMAND ${CMAKE_COMMAND} -E chmod +x ${CMAKE_BINARY_DIR}/app/loop.sh)
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
		get_filename_component(API_FILE_NAME ${API_CPP_FILE} NAME)

		if(IS_API_SUBDIR EQUAL 0 OR API_FILE_NAME STREQUAL "utils_api.cpp")
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