#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define TAG "zyh"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include <libavcodec/jni.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

static double r2d(AVRational rational) {
    return rational.num == 0 || rational.den == 0
           ? 0 : (double) rational.num / (double) rational.den;
}


//jni OnLoad 时调用
extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    //硬解码
    av_jni_set_java_vm(vm, nullptr);
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_vip_ruoyun_ffmpeg_XPlay_open(JNIEnv *env, jobject thiz, jstring url, jobject surface) {

    const char *path = env->GetStringUTFChars(url, 0);

    //初始化解封装
    av_register_all();
    //初始化网路
    avformat_network_init();
    //初始化解码器
    avcodec_register_all();

    //打开文件
    AVFormatContext *avFormatContext = nullptr;
    int result = avformat_open_input(&avFormatContext, path, nullptr, nullptr);
    if (result != 0) {
        LOGE("视频不能打开，原因 %s", av_err2str(result));
        return;
    }
    LOGE("视频能打开,duration = %lld nb_streams = %d ", avFormatContext->duration,
         avFormatContext->nb_streams);

    //获取流信息，flv 获取不到信息，需要 avformat_find_stream_info 来获取流信息
    result = avformat_find_stream_info(avFormatContext, nullptr);
    if (result != 0) {
        LOGE("视频 avformat_find_stream_info 获取失败");
    }
    LOGE("视频能打开,duration = %lld nb_streams = %d ", avFormatContext->duration,
         avFormatContext->nb_streams);


    double fps = 0;
    int videoStream = 0;
    int audioStream = 0;
    //取视频和音频
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *streams = avFormatContext->streams[i];
        //判断音频和视频
        if (streams->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { //视频
            videoStream = i;
            fps = r2d(streams->avg_frame_rate);
            LOGE("视频数据 ，fps = %f ,width  =%d height = %d codec_id =%d pixformat = %d",
                 fps,
                 streams->codecpar->width,
                 streams->codecpar->height,
                 streams->codecpar->codec_id,
                 streams->codecpar->format
            );

        } else if (streams->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) { //音频
            audioStream = i;
            LOGE("音频数据 ，sample_rate = %d ,channels  =%d  sample_format =%d",
                 streams->codecpar->sample_rate,
                 streams->codecpar->channels,
                 streams->codecpar->format
            );
        }
    }
    //获取音频流的索引，和上面的遍历差不多，更简单一些
    //audioStream = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    //视频解码器
    AVCodec *vCodec;

    //软解码器
    vCodec = avcodec_find_decoder(
            avFormatContext->streams[videoStream]->codecpar->codec_id);
//    vCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    //硬解码
    vCodec = avcodec_find_decoder_by_name("h264_mediacodec");


    if (!vCodec) {
        LOGE("avcodec not find");
        return;
    }

    //解码器初始化
    AVCodecContext *vCodecContext = avcodec_alloc_context3(vCodec);
    avcodec_parameters_to_context(vCodecContext, avFormatContext->streams[videoStream]->codecpar);
    vCodecContext->thread_count = 4;


    //打开解码器
    result = avcodec_open2(vCodecContext, nullptr, nullptr);
    if (result != 0) {
        LOGE("avcodec_open2 video failed!");
        return;
    }
    LOGE("avcodec_open2 video success!");

    //打开音频解码器
    //解码器
    AVCodec *acodec;

    //软解码器
    acodec = avcodec_find_decoder(
            avFormatContext->streams[audioStream]->codecpar->codec_id);

    //硬解码
//    codec = avcodec_find_decoder_by_name("h264_mediacodec");
//    codec = avcodec_find_decoder(AV_CODEC_ID_H264);

    if (!acodec) {
        LOGE("avcodec not find");
        return;
    }

    //解码器初始化
    AVCodecContext *aCodecContext = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(aCodecContext, avFormatContext->streams[audioStream]->codecpar);
    aCodecContext->thread_count = 2;


    //打开解码器
    result = avcodec_open2(aCodecContext, nullptr, nullptr);
    if (result != 0) {
        LOGE("avcodec_open2 audio failed!");
        return;
    }
    LOGE("avcodec_open2 audio success!");

    //读取帧数据
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();

    //视频
    SwsContext *swsContext = nullptr;
    int dstW = 1280;
    int dstH = 720;
    char *rgb = new char[1920 * 1080 * 4];

    //音频
    char *pcm = new char[48000 * 4 * 2];
    SwrContext *swrContext = swr_alloc();
    swrContext = swr_alloc_set_opts(swrContext,
                                    av_get_default_channel_layout(2),
                                    AV_SAMPLE_FMT_S16,
                                    aCodecContext->sample_rate,
                                    av_get_default_channel_layout(aCodecContext->channels),
                                    aCodecContext->sample_fmt,
                                    aCodecContext->sample_rate,
                                    0,
                                    nullptr
    );
    result = swr_init(swrContext);
    if (result != 0) {
        LOGE("swr_init failed!");
    } else {
        LOGE("swr_init success!");
    }

    //初始化显示窗口
    ANativeWindow *aNativeWindow = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_setBuffersGeometry(aNativeWindow, dstW, dstH, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer aNativeWindowBuffer;

    bool isStop = false;
    while (!isStop) {
        result = av_read_frame(avFormatContext, avPacket);
        if (result != 0) {
            LOGE("读取到结尾处");
            //seek到中间位置
            int pos = static_cast<int>(20 * r2d(avFormatContext->streams[videoStream]->time_base));
            av_seek_frame(avFormatContext, videoStream, pos,
                          AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
            continue;
        }
//        LOGE("stream = %d size = %d pts = %lld flag = %d",
//             avPacket->stream_index,
//             avPacket->size,
//             avPacket->pts,
//             avPacket->flags
//        );

        if (avPacket->stream_index == videoStream) {
            //视频
            //发送到线程中解码
            result = avcodec_send_packet(vCodecContext, avPacket);

            //清理
            av_packet_unref(avPacket);
            if (result != 0) {
                LOGE("avcodec_send_packet 失败");
                continue;
            }
            //接收视频解码
            while (true) {
                result = avcodec_receive_frame(vCodecContext, avFrame);
                if (result != 0) {
//                    LOGE("avcodec_receive_frame 失败");
                    break;
                }
                LOGE("avcodec_receive_frame %lld 成功", avFrame->pts);
                swsContext = sws_getCachedContext(
                        swsContext,
                        avFrame->width,
                        avFrame->height,
                        static_cast<AVPixelFormat>(avFrame->format),
                        dstW,
                        dstH,
                        AV_PIX_FMT_RGBA,
                        SWS_FAST_BILINEAR,
                        nullptr,
                        nullptr,
                        nullptr
                );

                if (!swsContext) {
                    LOGE("sws_getCachedContext failed");
                } else {
                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                    data[0] = (uint8_t *) rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = dstW * 4;
                    int height_sws_scale = sws_scale(swsContext,
                                                     avFrame->data,
                                                     avFrame->linesize,
                                                     0,
                                                     avFrame->height,
                                                     data,
                                                     lines
                    );
                    LOGE("height_sws_scale = %d", height_sws_scale);
                    //显示
                    if (height_sws_scale > 0) {
                        ANativeWindow_lock(aNativeWindow, &aNativeWindowBuffer, 0);
                        void *dst = aNativeWindowBuffer.bits;
                        memcpy(dst, rgb, dstW * dstH * 4);
                        ANativeWindow_unlockAndPost(aNativeWindow);
                    }
                    //释放
//                    sws_freeContext(swsContext);
                }
            }
        } else if (avPacket->stream_index == audioStream) {
            //音频解码
            //发送到线程中解码
            result = avcodec_send_packet(aCodecContext, avPacket);

            //清理
            av_packet_unref(avPacket);
            if (result != 0) {
                LOGE("avcodec_send_packet 失败");
                continue;
            }
            while (true) {
                //接收视频解码
                result = avcodec_receive_frame(aCodecContext, avFrame);
                if (result != 0) {
//                    LOGE("avcodec_receive_frame 失败");
                    break;
                }
                LOGE("avcodec_receive_frame %lld 成功", avFrame->pts);

                uint8_t *out[2] = {0};
                out[0] = (uint8_t *) pcm;

                //音频重采样
                int len = swr_convert(swrContext,
                                      out,
                                      avFrame->nb_samples,
                                      (const uint8_t **) avFrame->data,
                                      avFrame->nb_samples);
                LOGE("swr_convert = %d", len);
            }
        }
    }
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);

    delete[] rgb;
    delete[] pcm;

    sws_freeContext(swsContext);
    swr_free(&swrContext);

    //关闭流
    avformat_close_input(&avFormatContext);
    //释放
    env->ReleaseStringUTFChars(url, path);
}