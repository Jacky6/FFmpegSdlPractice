ffmpeg——SDL 学习项目记录
注意：所采用的ffmpeg sdl均为32位

### 食用方法
1、然后根据自己vs版本修改build.bat
2、运行build.bat生成项目
3、在build文件夹中打开VS项目*.sln文件


### 项目名称：ffmpeg_sdl_audio
##### 项目介绍
ffmpeg 采集麦克风 并通过 sdl 进行播放
包含模块：
+ 输出设备信息
+ 麦克风设备采集
+ 解码
+ SDL 音频播放


### 项目名称：ffmpeg_sdl_video
##### 项目介绍
ffmpeg 采集摄像头 并通过 sdl 进行播放
包含模块：
+ 输出设备信息
+ 设备采集
+ 解码
+ SDL 视频播放


### 项目名称：av_to_file
##### 项目介绍
ffmpeg 收集摄像头麦克风数据解码，然后编码，输出到mp4文件中
包含模块：
+ 设备读取
+ 解码，编码
+ 输出MP4
+ 多线程
+ ffmpeg自带音视频缓冲api


### 项目名称：av_to_sdl

