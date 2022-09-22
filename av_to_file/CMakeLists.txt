# CMakeList.txt: ffmpeg 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project ("av_to_file")


set (3rd_include_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/Libffmpeg/include
	${CMAKE_CURRENT_SOURCE_DIR}/SDL2/include
	)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/debug/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/debug/bin")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/debug/lib")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/debug/lib")

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")		# 程序输出文件为exe文件时起作用
set(CMAKE_SHARED_LINKR_FLAGS "${CMAKE_SHARED_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")		# 程序输出文件为dll文件时起作用
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	        # 程序输出文件为lib文件时起作用
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	# 程序输出文件为module文件时起作用
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})

include_directories(${3rd_include_dirs})

add_definitions(-D_ACTK_CLIENT_MACRO)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Libffmpeg/lib
				${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x86
				)
				
# 将源代码添加到此项目的可执行文件。
add_executable(main main.cpp)

target_link_libraries(main avcodec avdevice avfilter avformat avutil postproc swresample swscale SDL2 SDL2main)
  
set_property(TARGET main PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

# TODO: 如有需要，请添加测试并安装目标。
