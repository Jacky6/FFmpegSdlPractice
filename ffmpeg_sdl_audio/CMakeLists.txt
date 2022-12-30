
# 将源代码添加到此项目的可执行文件。
add_executable(ffmpeg_sdl_audio main.cpp)
target_link_libraries(ffmpeg_sdl_audio avcodec avdevice avfilter avformat avutil postproc swresample swscale SDL2 SDL2main)
  
set_property(TARGET ffmpeg_sdl_audio PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})

