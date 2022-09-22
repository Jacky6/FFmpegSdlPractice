#include <Windows.h>
#include <conio.h>
#include <iostream>

#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")

	//#pragma comment(lib, "avfilter.lib")
	//#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")
#ifdef __cplusplus
};
#endif

#define OUT_VIDEO_FORMAT AV_PIX_FMT_YUV420P

AVFormatContext* pFormatCtx_Audio = NULL, * pFormatCtx_Out = NULL;
AVCodecContext* pReadCodecContext_Audio = NULL;

AVFormatContext* pFormatCtx_Video = NULL;
AVCodecContext* pReadCodecCtx_Video = NULL;
AVCodec* pReadCodec_Video = NULL;

int VideoIndex = 0;
int AudioIndex_mic = 0;

AVCodecContext* pCodecEncodeCtx_Audio = NULL;
AVCodec* pCodecEncode_Audio = NULL;

AVCodecContext* pCodecEncodeCtx_Video = NULL;
AVCodec* pCodecEncode_Video = NULL;


AVFifoBuffer* fifo_video = NULL;
AVAudioFifo* fifo_audio_mic = NULL;

SwrContext* audio_convert_ctx = NULL;
SwsContext* img_convert_ctx = NULL;
int frame_size = 0;

uint8_t* picture_buf = NULL, * frame_buf = NULL;

int64_t cur_pts_v = 0;
int64_t cur_pts_a = 0;


CRITICAL_SECTION VideoSection;
CRITICAL_SECTION AudioSection;

DWORD WINAPI AudioMicCapThreadProc(LPVOID lpParam);
DWORD WINAPI ScreenCapThreadProc(LPVOID lpParam);

static char* dup_wchar_to_utf8(const wchar_t* w)
{
	char* s = NULL;
	int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
	s = (char*)av_malloc(l);
	if (s)
		WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
	return s;
}

/* just pick the highest supported samplerate */
static int select_sample_rate(const AVCodec* codec)
{
	const int* p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return 44100;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

/* select layout with the highest channel count */
static int select_channel_layout(const AVCodec* codec)
{
	const uint64_t* p;
	uint64_t best_ch_layout = 0;
	int best_nb_channels = 0;

	if (!codec->channel_layouts)
		return AV_CH_LAYOUT_STEREO;

	p = codec->channel_layouts;
	while (*p) {
		int nb_channels = av_get_channel_layout_nb_channels(*p);

		if (nb_channels > best_nb_channels) {
			best_ch_layout = *p;
			best_nb_channels = nb_channels;
		}
		p++;
	}
	return best_ch_layout;
}

int OpenVideoCapture()
{
	AVInputFormat* ifmt = av_find_input_format("dshow");
	AVDictionary* options = NULL;
	av_dict_set(&options, "framerate", "30", NULL);
	av_dict_set(&options,"vcodec","mjpeg",0);
	av_dict_set(&options,"rtbufsize","100*1280*720",0);
	//av_dict_set(&options, "probesize", "50000000", NULL);
	//av_dict_set(&options,"offset_x","20",0);
	//The distance from the top edge of the screen or desktop
	//av_dict_set(&options,"offset_y","40",0);
	//Video frame size. The default is to capture the full screen
	
	/* 可以自行查找摄像头设备名称 */
	if (avformat_open_input(&pFormatCtx_Video, "video=Integrated Camera", ifmt, &options) != 0)
	{
		std::cout << "Couldn't open input stream.（无法打开视频输入流）\n";
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx_Video, NULL) < 0)
	{
		std::cout << "Couldn't find stream information.（无法获取视频流信息）\n";
		return -1;
	}
	if (pFormatCtx_Video->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
	{
		std::cout << "Couldn't find video stream information.（无法获取视频流信息）\n";
		return -1;
	}

	/* 配置解码器 */
	pReadCodec_Video = (AVCodec*)avcodec_find_decoder(pFormatCtx_Video->streams[0]->codecpar->codec_id);
	pReadCodecCtx_Video = avcodec_alloc_context3(pReadCodec_Video);
	if (pReadCodec_Video == NULL)
	{
		std::cout << "Codec not found.（没有找到解码器）\n";
		return -1;
	}
	avcodec_parameters_to_context(pReadCodecCtx_Video, pFormatCtx_Video->streams[0]->codecpar);
	/* frames per second */
	AVRational timebase;
	timebase.den = 30;
	timebase.num = 1;
	pReadCodecCtx_Video->time_base = timebase;
	AVRational frameRate;
	frameRate.den = 1;
	frameRate.num = 30;
	pReadCodecCtx_Video->framerate = frameRate;
	pReadCodecCtx_Video->pix_fmt = (AVPixelFormat)pFormatCtx_Video->streams[0]->codecpar->format;

	if (avcodec_open2(pReadCodecCtx_Video, pReadCodec_Video, NULL) < 0)
	{
		std::cout << "Could not open codec.（无法打开解码器）\n";
		return -1;
	}


	img_convert_ctx = sws_getContext(pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, pReadCodecCtx_Video->pix_fmt,
		pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, OUT_VIDEO_FORMAT, SWS_BICUBIC, NULL, NULL, NULL);

	fifo_video = av_fifo_alloc(30 * av_image_get_buffer_size(OUT_VIDEO_FORMAT, pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, 1));

	return 0;
}

int OpenAudioCapture()
{
	//查找输入方式
	AVInputFormat* imf = av_find_input_format("dshow");
	//以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
	const char* psDevName = u8"audio=麦克风 (2- Realtek(R) Audio)";//dup_wchar_to_utf8(L"audio=麦克风(2 - Realtek(R) Audio)");
	AVDictionary* options = NULL;
	//av_dict_set(&options, "framerate", "30", NULL);
	if (avformat_open_input(&pFormatCtx_Audio, psDevName, imf, &options) < 0)
	{
		printf("Couldn't open input stream.（无法打开音频输入流）\n");
		return -1;
	}

	if (pFormatCtx_Audio->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
	{
		printf("Couldn't find video stream information.（无法获取音频流信息）\n");
		return -1;
	}


	const AVCodec* tmpCodec = avcodec_find_decoder(pFormatCtx_Audio->streams[0]->codecpar->codec_id);

	pReadCodecContext_Audio = avcodec_alloc_context3(tmpCodec);

	pReadCodecContext_Audio->sample_rate = select_sample_rate(tmpCodec);
	pReadCodecContext_Audio->channel_layout = select_channel_layout(tmpCodec);
	pReadCodecContext_Audio->channels = av_get_channel_layout_nb_channels(pReadCodecContext_Audio->channel_layout);

	pReadCodecContext_Audio->sample_fmt = (AVSampleFormat)pFormatCtx_Audio->streams[0]->codecpar->format;

	if (0 > avcodec_open2(pReadCodecContext_Audio, tmpCodec, NULL))
	{
		printf("can not find or open audio decoder!\n");
	}


	audio_convert_ctx = swr_alloc();
	av_opt_set_channel_layout(audio_convert_ctx, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_channel_layout(audio_convert_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(audio_convert_ctx, "in_sample_rate", 44100, 0);
	av_opt_set_int(audio_convert_ctx, "out_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(audio_convert_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	av_opt_set_sample_fmt(audio_convert_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

	swr_init(audio_convert_ctx);


	return 0;
}

int OpenOutPut()
{
	AVStream* pVideoStream = NULL;
	AVStream* pAudioStream = NULL;
	const char* outFileName = "test.mp4";
	avformat_alloc_output_context2(&pFormatCtx_Out, NULL, NULL, outFileName);

	/* 添加视频流 */
	if (pFormatCtx_Video->streams[0]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		VideoIndex = 0;
		pVideoStream = avformat_new_stream(pFormatCtx_Out, NULL);
		if (!pVideoStream)
		{
			std::cout << "can not new stream for output!\n";
			return -1;
		}

		/* 配置视频编码器 */
		AVRational timeBase;
		timeBase.num = 1;
		timeBase.den = 30;
		pVideoStream->time_base = timeBase;
		pCodecEncode_Video = (AVCodec*)avcodec_find_encoder(pFormatCtx_Out->oformat->video_codec);
		if (!(pCodecEncode_Video)) {
			std::cout << "Could not find encoder for video\n";
			return -1;
		}
		pCodecEncodeCtx_Video = avcodec_alloc_context3(pCodecEncode_Video);
		if (!pCodecEncodeCtx_Video) {
			fprintf(stderr, "Could not alloc an encoding context\n");
			exit(1);
		}
		pCodecEncodeCtx_Video->time_base = timeBase;
		pCodecEncodeCtx_Video->codec_id = pFormatCtx_Out->oformat->video_codec;
		pCodecEncodeCtx_Video->bit_rate = 400000;
		pCodecEncodeCtx_Video->width = pReadCodecCtx_Video->width;
		pCodecEncodeCtx_Video->height = pReadCodecCtx_Video->height;
		// pCodecEncodeCtx_Video->gop_size = 25; /* emit one intra frame every twelve frames at most */
		pCodecEncodeCtx_Video->pix_fmt = OUT_VIDEO_FORMAT;

		av_opt_set(pCodecEncodeCtx_Video->priv_data, "profile", "main", 0); // 避免编码占用
		av_opt_set(pCodecEncodeCtx_Video->priv_data, "preset", "fast", 0);
		av_opt_set(pCodecEncodeCtx_Video->priv_data, "tune", "zerolatency", 0); // 避免编码器堵塞
		if ((avcodec_open2(pCodecEncodeCtx_Video, pCodecEncode_Video, NULL)) < 0)
		{
			std::cout << "can not open the encoder\n";
			return -1;
		}
	}

	if (pFormatCtx_Audio->streams[0]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		AVCodecContext* pOutputCodecCtx;
		pAudioStream = avformat_new_stream(pFormatCtx_Out, NULL);

		AudioIndex_mic = 1;

		pCodecEncode_Audio = (AVCodec*)avcodec_find_encoder(pFormatCtx_Out->oformat->audio_codec);

		pCodecEncodeCtx_Audio = avcodec_alloc_context3(pCodecEncode_Audio);
		if (!pCodecEncodeCtx_Audio) {
			fprintf(stderr, "Could not alloc an encoding context\n");
			exit(1);
		}

		//pCodecEncodeCtx_Audio->codec_id = pFormatCtx_Out->oformat->audio_codec;
		pCodecEncodeCtx_Audio->sample_fmt = pCodecEncode_Audio->sample_fmts ? pCodecEncode_Audio->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
		pCodecEncodeCtx_Audio->bit_rate = 64000;
		pCodecEncodeCtx_Audio->sample_rate = pReadCodecContext_Audio->sample_rate;
		if (pCodecEncode_Audio->supported_samplerates) {
			pCodecEncodeCtx_Audio->sample_rate = pCodecEncode_Audio->supported_samplerates[0];
			for (int i = 0; pCodecEncode_Audio->supported_samplerates[i]; i++) {
				if (pCodecEncode_Audio->supported_samplerates[i] == 44100)
					pCodecEncodeCtx_Audio->sample_rate = 44100;
			}
		}
		pCodecEncodeCtx_Audio->channels = av_get_channel_layout_nb_channels(pCodecEncodeCtx_Audio->channel_layout);
		pCodecEncodeCtx_Audio->channel_layout = AV_CH_LAYOUT_STEREO;
		if (pCodecEncode_Audio->channel_layouts) {
			pCodecEncodeCtx_Audio->channel_layout = pCodecEncode_Audio->channel_layouts[0];
			for (int i = 0; pCodecEncode_Audio->channel_layouts[i]; i++) {
				if (pCodecEncode_Audio->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
					pCodecEncodeCtx_Audio->channel_layout = AV_CH_LAYOUT_STEREO;
			}
		}
		pCodecEncodeCtx_Audio->channels = av_get_channel_layout_nb_channels(pCodecEncodeCtx_Audio->channel_layout);


		AVRational timeBase;
		timeBase.den = pCodecEncodeCtx_Audio->sample_rate;
		timeBase.num = 1;
		pAudioStream->time_base = timeBase;

		if (avcodec_open2(pCodecEncodeCtx_Audio, pCodecEncode_Audio, 0) < 0)
		{
			//编码器打开失败，退出程序
			return -1;
		}
	}


	if (!(pFormatCtx_Out->oformat->flags & AVFMT_NOFILE))
	{
		if (avio_open(&pFormatCtx_Out->pb, outFileName, AVIO_FLAG_WRITE) < 0)
		{
			printf("can not open output file handle!\n");
			return -1;
		}
	}

	avcodec_parameters_from_context(pVideoStream->codecpar, pCodecEncodeCtx_Video);
	avcodec_parameters_from_context(pAudioStream->codecpar, pCodecEncodeCtx_Audio);

	if (avformat_write_header(pFormatCtx_Out, NULL) < 0) // ?? 这里会自动修改视频timebase
	{
		printf("can not write the header of the output file!\n");
		return -1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16;
	int iSize = av_get_bytes_per_sample(sample_fmt);
	avdevice_register_all();

	if (OpenVideoCapture() < 0)
	{
		return -1;
	}

	if (OpenAudioCapture() < 0)
	{
		return -1;
	}

	if (OpenOutPut() < 0)
	{
		return -1;
	}

	InitializeCriticalSection(&VideoSection);//初始化临界资源
	InitializeCriticalSection(&AudioSection);

	AVFrame* pFrameYUVInMain = av_frame_alloc();
	frame_size = av_image_get_buffer_size(OUT_VIDEO_FORMAT, pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, 1);
	uint8_t* out_buffer_yuv420 = (uint8_t*)av_malloc(frame_size);
	av_image_fill_arrays(pFrameYUVInMain->data, pFrameYUVInMain->linesize, out_buffer_yuv420, 
		OUT_VIDEO_FORMAT, pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, 1);

	int AudioFrameIndex = 0;
	int VideoFrameIndex = 0;
	AVPacket packet = { 0 };
	CreateThread(NULL, 0, ScreenCapThreadProc, 0, 0, NULL);
	CreateThread(NULL, 0, AudioMicCapThreadProc, 0, 0, NULL);
	
	while (VideoFrameIndex <= 100)
	{
		if (NULL == fifo_audio_mic)
		{
			continue;
		}
		if (av_compare_ts(cur_pts_v, pFormatCtx_Out->streams[VideoIndex]->time_base, cur_pts_a, pFormatCtx_Out->streams[AudioIndex_mic]->time_base) <= 0)
		{
			if (av_fifo_size(fifo_video) >= frame_size)
			{
				EnterCriticalSection(&VideoSection);
				av_fifo_generic_read(fifo_video, out_buffer_yuv420, frame_size, NULL);
				LeaveCriticalSection(&VideoSection);

				pFrameYUVInMain->width = pReadCodecCtx_Video->width;
				pFrameYUVInMain->height = pReadCodecCtx_Video->height;
				pFrameYUVInMain->format = OUT_VIDEO_FORMAT;
				pFrameYUVInMain->pts = VideoFrameIndex;
				pFrameYUVInMain->pkt_dts = VideoFrameIndex;			// 对应time_base = 1/30
				
				av_packet_unref(&packet);
				ret = avcodec_send_frame(pCodecEncodeCtx_Video, pFrameYUVInMain);
				ret = avcodec_receive_packet(pCodecEncodeCtx_Video, &packet);

				av_packet_rescale_ts(&packet, pReadCodecCtx_Video->time_base, pFormatCtx_Out->streams[VideoIndex]->time_base);
				packet.duration = av_rescale_q(1, pReadCodecCtx_Video->time_base, pFormatCtx_Out->streams[VideoIndex]->time_base);
				cur_pts_v = packet.pts;

				ret = av_interleaved_write_frame(pFormatCtx_Out, &packet);
				if (ret < 0) {
					std::cout << VideoFrameIndex << "video write error!\n";
				}
				else{
					std::cout << VideoFrameIndex << "video write success!\n";
					VideoFrameIndex++;
				}
				avio_flush(pFormatCtx_Out->pb);
			}
		}
		else
		{
			if (av_audio_fifo_size(fifo_audio_mic) >= 1024)
			{
				AVFrame* frame_mic = NULL;
				frame_mic = av_frame_alloc();
				frame_mic->nb_samples = 1024;
				frame_mic->channels = 2;
				frame_mic->channel_layout = av_get_default_channel_layout(frame_mic->channels);
				frame_mic->format = pFormatCtx_Audio->streams[0]->codecpar->format;
				frame_mic->sample_rate = pFormatCtx_Audio->streams[0]->codecpar->sample_rate;
				av_frame_get_buffer(frame_mic, 0);

				EnterCriticalSection(&AudioSection);
				int readcount = av_audio_fifo_read(fifo_audio_mic, (void**)frame_mic->data, 1024);
				LeaveCriticalSection(&AudioSection);

				AVPacket pkt_out_mic = { 0 };
				int got_picture_mic = -1;
				pkt_out_mic.data = NULL;
				pkt_out_mic.size = 0;
				frame_mic->pts = AudioFrameIndex;

				AVFrame* frame_mic_encode = NULL;
				frame_mic_encode = av_frame_alloc();
				frame_mic_encode->nb_samples = pCodecEncodeCtx_Audio->frame_size;
				frame_mic_encode->channel_layout = pCodecEncodeCtx_Audio->channel_layout;
				frame_mic_encode->format = pCodecEncodeCtx_Audio->sample_fmt;
				frame_mic_encode->sample_rate = pCodecEncodeCtx_Audio->sample_rate;
				av_frame_get_buffer(frame_mic_encode, 0);
				int dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_convert_ctx, frame_mic->sample_rate) + frame_mic->nb_samples, frame_mic->sample_rate, frame_mic->sample_rate, AVRounding(1));

				//uint8_t *audio_buf = NULL;
				uint8_t* audio_buf[2] = { 0 };
				audio_buf[0] = (uint8_t*)frame_mic_encode->data[0];
				audio_buf[1] = (uint8_t*)frame_mic_encode->data[1];
				frame_mic_encode->nb_samples = swr_convert(audio_convert_ctx, audio_buf, dst_nb_samples, (const uint8_t**)frame_mic->data, frame_mic->nb_samples);

				ret = avcodec_send_frame(pCodecEncodeCtx_Audio, frame_mic_encode);
				ret = avcodec_receive_packet(pCodecEncodeCtx_Audio, &pkt_out_mic);

				pkt_out_mic.stream_index = AudioIndex_mic;
				pkt_out_mic.pts = av_rescale_q(AudioFrameIndex* frame_mic_encode->nb_samples, pReadCodecContext_Audio->time_base, pFormatCtx_Out->streams[AudioIndex_mic]->time_base);
				pkt_out_mic.dts = av_rescale_q(AudioFrameIndex* frame_mic_encode->nb_samples, pReadCodecContext_Audio->time_base, pFormatCtx_Out->streams[AudioIndex_mic]->time_base);
				pkt_out_mic.duration = frame_mic_encode->nb_samples;
				cur_pts_a = pkt_out_mic.pts;


				int ret2 = av_interleaved_write_frame(pFormatCtx_Out, &pkt_out_mic);
				if (ret < 0) {
					printf("audio write error!\n");
				}
				else {
					printf("audio write success!\n");
				}

				av_packet_unref(&pkt_out_mic);
				av_frame_free(&frame_mic);
				av_frame_free(&frame_mic_encode);

				AudioFrameIndex++;
			}
		}
	}

	av_write_trailer(pFormatCtx_Out);

	avio_close(pFormatCtx_Out->pb);
	avformat_free_context(pFormatCtx_Out);

	if (pFormatCtx_Audio != NULL)
	{
		avformat_close_input(&pFormatCtx_Audio);
		pFormatCtx_Audio = NULL;
	}

	return 0;
}

DWORD WINAPI AudioMicCapThreadProc(LPVOID lpParam)
{
	AVFrame* pFrame;
	pFrame = av_frame_alloc();

	AVPacket packet = { 0 };
	int ret = 0;
	while (1)
	{
		av_packet_unref(&packet);
		if (av_read_frame(pFormatCtx_Audio, &packet) < 0)
		{
			continue;
		}
		ret = avcodec_send_packet(pReadCodecContext_Audio, &packet);
		if (ret >= 0)
		{
			ret = avcodec_receive_frame(pReadCodecContext_Audio, pFrame);
			if (ret == AVERROR(EAGAIN))
			{
				break;
			}
			else if (ret == AVERROR_EOF)
			{
				return 0;
			}
			else if (ret < 0) {
				std::cout << "Error during the audio decoding\n";
				exit(1);
			}

			if (NULL == fifo_audio_mic)
			{
				fifo_audio_mic = av_audio_fifo_alloc((AVSampleFormat)pFormatCtx_Audio->streams[0]->codecpar->format,
					pFormatCtx_Audio->streams[0]->codecpar->channels, 30 * pFrame->nb_samples);
			}

			int buf_space = av_audio_fifo_space(fifo_audio_mic);
			if (av_audio_fifo_space(fifo_audio_mic) >= pFrame->nb_samples)
			{
				EnterCriticalSection(&AudioSection);
				ret = av_audio_fifo_write(fifo_audio_mic, (void**)pFrame->data, pFrame->nb_samples);
				LeaveCriticalSection(&AudioSection);
			}
			av_packet_unref(&packet);
		}

	}

	return 0;
}

DWORD WINAPI ScreenCapThreadProc(LPVOID lpParam)
{
	AVFrame* pFrame;
	pFrame = av_frame_alloc();

	AVFrame* pFrameYUV = av_frame_alloc(); // yuv帧的大小 3110400 = Y(w*h)+U(0.5*w*h)+V(0)   w:1920 h:1080
	int frame_size = av_image_get_buffer_size(OUT_VIDEO_FORMAT, pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, 1);
	uint8_t* out_buffer_yuv420 = (uint8_t*)av_malloc(frame_size);
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, 
		out_buffer_yuv420, 
		OUT_VIDEO_FORMAT, 
		pReadCodecCtx_Video->width, pReadCodecCtx_Video->height, 1);

	int ret = 0;
	AVPacket packet = { 0 };
	int y_size = pReadCodecCtx_Video->width * pReadCodecCtx_Video->height;
	while (1)
	{
		av_packet_unref(&packet);//释放packet内存
		if (av_read_frame(pFormatCtx_Video, &packet) < 0)
		{
			std::cout << "get one video packet failed!" << std::endl;
			continue;
		}
		ret = avcodec_send_packet(pReadCodecCtx_Video, &packet);
		if (ret >= 0)
		{
			ret = avcodec_receive_frame(pReadCodecCtx_Video, pFrame);
			if (ret == AVERROR(EAGAIN))
			{
				continue;
			}
			else if (ret == AVERROR_EOF)
			{
				break;
			}
			else if (ret < 0) {
				std::cout << "Error during decoding\n" ;
				break;
			}


			int iScale = sws_scale(img_convert_ctx, 
				(const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecEncodeCtx_Video->height,
				pFrameYUV->data, pFrameYUV->linesize);

			if (av_fifo_space(fifo_video) >= frame_size)
			{
				EnterCriticalSection(&VideoSection);
				av_fifo_generic_write(fifo_video, pFrameYUV->data[0], y_size, NULL);
				av_fifo_generic_write(fifo_video, pFrameYUV->data[1], y_size / 4, NULL);
				av_fifo_generic_write(fifo_video, pFrameYUV->data[2], y_size / 4, NULL);
				LeaveCriticalSection(&VideoSection);
			}
		}
		if (ret == AVERROR(EAGAIN))
		{
			continue;
		}
	}

	av_frame_free(&pFrame);
	av_frame_free(&pFrameYUV);
	return 0;
}
