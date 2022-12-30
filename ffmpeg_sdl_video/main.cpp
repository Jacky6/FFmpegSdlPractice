
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
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
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

	AVFormatContext* pFormatCtx;
	unsigned int i;
	int videoindex;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;

	pFormatCtx = avformat_alloc_context();
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
	av_dict_set(&options, "video_size", "1280x720", 0);
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "vcodec", "mjpeg", 0);
	//Set own video device's name
	if (avformat_open_input(&pFormatCtx, "video=Integrated Camera", ifmt, &options) != 0) {//"video=Integrated Camera"
		printf("Couldn't open input stream.\n");
		return -1;
	}
	printf("size:%s \n", pFormatCtx->iformat->name);
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
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;

	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	if (videoindex == -1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}

	pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
	pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);
	//printf("解码器：%s\n", pCodec->name);
	//printf("输出分辨率：w:%d,h:%d\n", pCodecCtx->width, pCodecCtx->height);

	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	AVFrame* pFrame, * pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	SDL_Rect sdlRect;
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = pCodecCtx->width;
	sdlRect.h = pCodecCtx->height;
	//uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	// 
	//SDL----------------------------
	//1、初始化SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//2、创建SDL_Window：SDL_CreateWindow()
	int screen_w = 0, screen_h = 0;
	SDL_Window* screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;

	screen = SDL_CreateWindow("My Game Window",	//窗口名
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		screen_w, screen_h,
		0);
	if (!screen) {
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
		return -1;
	}
	//创建渲染器和纹理
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_Texture* sdlTexture = SDL_CreateTexture(
		sdlRenderer,
		SDL_PIXELFORMAT_YV12,
		SDL_TEXTUREACCESS_STREAMING,
		pCodecCtx->width,
		pCodecCtx->height);

	//SDL End--------
	int ret;

	struct SwsContext* img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);

	int numBytes = av_image_get_buffer_size(
		AV_PIX_FMT_YUV420P,
		pCodecCtx->width,
		pCodecCtx->height,
		1);
	uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	av_image_fill_arrays(pFrameYUV->data,pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P,
		pCodecCtx->width, pCodecCtx->height, 1);
	//------------------------------
	AVPacket *packet;
	SDL_Event event;
	for (;;) {
		packet = av_packet_alloc();
		if (av_read_frame(pFormatCtx, packet) >= 0) {
			// Is this a packet from the video stream?
			if (packet->stream_index == videoindex) {
				// Decode video frame
				int frameFinished = 1;
				avcodec_send_packet(pCodecCtx, packet);
				ret = avcodec_receive_frame(pCodecCtx, pFrame);
				if (ret < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
				
				// Did we get a video frame?
				if (frameFinished) {
					
					sws_scale(
						img_convert_ctx,
						(uint8_t const* const*)pFrame->data,
						pFrame->linesize,
						0,
						pCodecCtx->height,
						pFrameYUV->data,
						pFrameYUV->linesize
					);

					SDL_UpdateTexture(sdlTexture, &sdlRect, pFrameYUV->data[0], pFrameYUV->linesize[0]);
					SDL_RenderClear(sdlRenderer);
					SDL_RenderCopy(sdlRenderer, sdlTexture, &sdlRect, &sdlRect);
					SDL_RenderPresent(sdlRenderer);
				}
				SDL_Delay(5);
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
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);//关闭之后就不用free了
	return 0;
}