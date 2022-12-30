cmake_minimum_required (VERSION 3.8)
project(FFMPEG_SDL_PRACTICE CXX)

## sdl\ffmpeg 库和头文件
set(3rd_include_dirs
	${CMAKE_CURRENT_SOURCE_DIR}/3rdLib/Libffmpeg/include
	${CMAKE_CURRENT_SOURCE_DIR}/3rdLib/SDL2/include
	)

set(3rd_lib_dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdLib/Libffmpeg/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdLib/SDL2/lib/x86
    )

message(STATUS "3rd include :" ${3rd_include_dirs})
message(STATUS "3rd lib :" ${3rd_lib_dirs})
include_directories(${3rd_include_dirs})
link_directories(${3rd_lib_dirs})

## 项目配置
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
# set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
# set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib")
link_directories(${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")			# 程序输出文件为exe文件时起作用
# set(CMAKE_SHARED_LINKR_FLAGS "${CMAKE_SHARED_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")		# 程序输出文件为dll文件时起作用
# set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKR_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	    # 程序输出文件为lib文件时起作用
# set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libc.lib")	# 程序输出文件为module文件时起作用

add_subdirectory(av_to_file)
add_subdirectory(av_to_sdl)
add_subdirectory(ffmpeg_sdl_audio)
add_subdirectory(ffmpeg_sdl_video)

# 设置为默认启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT av_to_file)