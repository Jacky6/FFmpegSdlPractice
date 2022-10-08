
#include <iostream>
#define __STDC_CONSTANT_MACROS //它允许C++程序使用C99标准中指定的 stdint.h 宏，而这些宏不在C++标准中。 
//诸如 UINT8_MAX ， INT64_MIN 和 INT32_C () 之类的宏可能已在C++应用程序中以其他方式定义

extern "C" {

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"
#include "SDL_main.h"
#include "SDL_video.h"
#include "SDL_thread.h"
#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif
}


#include <string>

//Output YUV420P 
#define OUTPUT_YUV420P 0
//'1' Use Dshow 
//'0' Use VFW
#define USE_DSHOW 1
//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

//Show Dshow Device
void show_dshow_device() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0); //0表示不区分大小写
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx, "audio=dummy", iformat, &options);
	printf("================================\n");
	avformat_free_context(pFormatCtx);
}
//Show Dshow Device Option
void show_dshow_device_option() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx, "video=Integrated Camera", iformat, &options);
	printf("================================\n");
	avformat_free_context(pFormatCtx);
}

//Show VFW Device
void show_vfw_device() {
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVInputFormat* iformat = av_find_input_format("vfwcap");
	printf("========VFW Device Info======\n");
	avformat_open_input(&pFormatCtx, "list", iformat, NULL);
	printf("=============================\n");
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);
}

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

int thread_exit = 0;

int sfp_refresh_thread(void* opaque)
{
	while (thread_exit == 0) {
		SDL_Event event;
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}
	return 0;
}

int main()
{
	av_log_set_level(0);
	AVFormatContext* pFormatCtx;
	pFormatCtx = avformat_alloc_context();
	unsigned int i;
	int audioindex;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;

	avdevice_register_all();
	//Windows
	//Show Dshow Device
	show_dshow_device();
	//Show Device Options
	show_dshow_device_option();
	//Show VFW Options
	show_vfw_device();

#if USE_DSHOW
	AVInputFormat* ifmt = av_find_input_format("dshow");
	AVDictionary* options = NULL;
	//av_dict_set(&options, "rtbufsize", "4*1920*1080", 0);
	//Set own video device's name
	if (avformat_open_input(&pFormatCtx, u8"audio=麦克风 (2- Realtek(R) Audio)", ifmt, &options) != 0) {//"video=Integrated Camera"
		std::cout << "Couldn't open input stream.\n";
		return -1;
	}
#else
	AVInputFormat* ifmt = av_find_input_format("vfwcap");
	//AVDictionary* options;
	//av_dict_set_int(&options, "rtbufsize", 3041280 * 100, 0);//默认大小3041280
	if (avformat_open_input(&pFormatCtx, "0", ifmt, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
#endif
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		std::cout << "Couldn't find stream information.\n";
		return -1;
	}
	
	audioindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioindex = i;
			break;
		}
	if (audioindex == -1)
	{
		std::cout << "Couldn't find a video stream.\n";
		return -1;
	}

	pCodec = avcodec_find_decoder(pFormatCtx->streams[audioindex]->codecpar->codec_id);
	pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audioindex]->codecpar);
	std::cout << "解码器：" <<  pCodec->name << std::endl;

	if (pCodec == NULL)
	{
		std::cout << "Codec not found.\n";
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		std::cout << "Could not open codec.\n";
		return -1;
	}



	/* SDL 配置 */
	// 1、初始化SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// 2、音频播放参数及打开音频设备
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = pCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = pCodecCtx->channels;
	wanted_spec.silence = 0;	// 设置静音的值
	wanted_spec.samples = 1024;
	wanted_spec.callback = nullptr; // 推送模式播放音频
	wanted_spec.userdata = NULL;
	SDL_AudioDeviceID deviceId;
	if (deviceId = SDL_OpenAudioDevice(nullptr, 0, &wanted_spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE) < 2)
	{
		std::cout << "open audio device failed!\n";
		return -1;
	}
	// 3、暂停播放
	SDL_PauseAudioDevice(deviceId, 1);


	/* 根据SDL音频播放格式进行重采样 */
	SwrContext* audio_resample_ctx = NULL;
	audio_resample_ctx = swr_alloc();
	swr_alloc_set_opts(audio_resample_ctx,
		pCodecCtx->channel_layout, AV_SAMPLE_FMT_S16, pCodecCtx->sample_rate,
		pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
		0, NULL);
	swr_init(audio_resample_ctx);

	AVFrame* pFrame, * pFrameOut = NULL;
	pFrame = av_frame_alloc();
	pFrameOut = av_frame_alloc();


	/* 播放 */
	SDL_PauseAudioDevice(deviceId, 0);
	AVPacket *packet;
	SDL_Event event;
	for (;;) {
		packet = av_packet_alloc();
		if (av_read_frame(pFormatCtx, packet) >= 0) {
			// Is this a packet from the video stream?
			if (packet->stream_index == audioindex) {
				// Decode video frame
				int frameFinished = 1;
				avcodec_send_packet(pCodecCtx, packet);
				if (avcodec_receive_frame(pCodecCtx, pFrame) < 0) {
					std::cout << "Decode Error.\n";
					return -1;
				}
				
				// 重采样
				av_frame_unref(pFrameOut);
				pFrameOut->format = AV_SAMPLE_FMT_S16;
				pFrameOut->channels = pCodecCtx->channels;
				pFrameOut->channel_layout = pCodecCtx->channel_layout;
				pFrameOut->sample_rate = pCodecCtx->sample_rate;
				pFrameOut->nb_samples = pFrame->nb_samples;
				av_frame_get_buffer(pFrameOut,0);

				pFrameOut->nb_samples = swr_convert_frame(audio_resample_ctx, pFrameOut, pFrame);
				if (pFrameOut->nb_samples < 0) {
					std::cout << "resample failed!\n";
					return -1;
				}
				int data_len = av_samples_get_buffer_size(pFrameOut->linesize, pFrameOut->channels, pFrameOut->nb_samples, (AVSampleFormat)pFrameOut->format, 0);
				// 推送数据给sdl
				SDL_QueueAudio(deviceId, pFrameOut->data[0], data_len);
			}
			// Free the packet that was allocated by av_read_frame
			av_packet_free(&packet);
			SDL_PollEvent(&event);
			switch (event.type) {
			case SDL_QUIT:
				SDL_Quit();
				exit(0);
				break;
			default:
				break;
			}
		}
	}
	//av_free(out_buffer);
	av_frame_free(&pFrame);
	av_free(pFrameOut);
	swr_free(&audio_resample_ctx);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);//关闭之后就不用free了

	SDL_CloseAudioDevice(deviceId);

	return 0;
}