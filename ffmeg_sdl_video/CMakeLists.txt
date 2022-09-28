cmake_minimum_required (VERSION 3.8)

project ("ffmpeg_sdl_video")

## 第三方库和头文件
set(3rd_include_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/Libffmpeg/include
	${CMAKE_CURRENT_SOURCE_DIR}/SDL2/include
	)

include_directories(${3rd_include_dirs})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/Libffmpeg/lib
				${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x86
				)

## 项目配置
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/debug/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/debug/bin")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/debug/lib")
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/debug/lib")
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")			# 程序输出文件为exe文件时起作用
set(CMAKE_SHARED_LINKR_FLAGS "${CMAKE_SHARED_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")		# 程序输出文件为dll文件时起作用
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	    # 程序输出文件为lib文件时起作用
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	# 程序输出文件为module文件时起作用

## 
add_definitions(-D_ACTK_CLIENT_MACRO)
		
# 将源代码添加到此项目的可执行文件。
add_executable(main main.cpp)
target_link_libraries(main avcodec avdevice avfilter avformat avutil postproc swresample swscale SDL2 SDL2main)
  
set_property(TARGET main PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
# 设置为默认启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)

# TODO: 如有需要，请添加测试并安装目标。
