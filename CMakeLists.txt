cmake_minimum_required(VERSION 3.15)

project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_API_TEST "Build Api Test Executables" ON)

# 编译项目编写的API测试代码
if(BUILD_API_TEST)
	# 定义源文件目录
	set(API_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/api_test)

	# 查找所有的cpp文件
	file(GLOB_RECURSE API_CPP_FILES ${API_TEST_DIR}/*.cpp)

	# 设置生成的可执行文件输出目录
	set(API_EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/api_test)

	# 为每个cpp文件生成一个可执行文件
	foreach(API_CPP_FILE ${API_CPP_FILES})
		# 获取文件名（不带路径和扩展名）
		get_filename_component(FILE_NAME ${API_CPP_FILE} NAME_WE)
		
		# 获取相对于API_TEST_DIR的路径
		file(RELATIVE_PATH REL_PATH ${API_TEST_DIR} ${API_CPP_FILE})
		
		# 检查是否在api/目录下
		string(FIND ${REL_PATH} "api/" IS_API_SUBDIR)
		
		if(IS_API_SUBDIR EQUAL 0)
			# 在api/目录下，需要添加对应的src文件
			string(REPLACE "Api" "" SRC_FILE_NAME ${FILE_NAME})
			set(SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/${SRC_FILE_NAME}.cpp)
			
			# 检查对应的src文件是否存在
			if(EXISTS ${SRC_FILE})
				add_executable(${FILE_NAME} ${API_CPP_FILE} ${SRC_FILE})
			else()
				add_executable(${FILE_NAME} ${API_CPP_FILE})
			endif()
            # 设置 api/ 目录下可执行文件的输出目录
            set_target_properties(${FILE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${API_EXECUTABLE_OUTPUT_PATH}/api)
		else()
			# 不在api/目录下，正常处理
			add_executable(${FILE_NAME} ${API_CPP_FILE})
            # 设置其它可执行文件的输出目录
            set_target_properties(${FILE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${API_EXECUTABLE_OUTPUT_PATH})
		endif()
	endforeach()
endif()