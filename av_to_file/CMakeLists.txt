
# 将源代码添加到此项目的可执行文件。
add_executable(av_to_file main.cpp)
target_link_libraries(av_to_file avcodec avdevice avfilter avformat avutil postproc swresample swscale SDL2 SDL2main)
  
set_property(TARGET av_to_file PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})


