
# 将源代码添加到此项目的可执行文件。
add_executable(av_to_sdl main.cpp)
target_link_libraries(av_to_sdl avcodec avdevice avfilter avformat avutil postproc swresample swscale SDL2 SDL2main)
  
set_property(TARGET av_to_sdl PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

