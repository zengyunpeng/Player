//
// Created by Administrator on 2019/6/26.
//
#include <cstring>
#include <pthread.h>
#include <chrono>
#include "DNFFmpeg.h"
#include "macro.h"


void *task_prepare(void *args) {
    DNFFmpeg *fFmpeg = static_cast<DNFFmpeg *>(args);
    fFmpeg->_prepare();
    return 0;
}

DNFFmpeg::DNFFmpeg(JavaCallHelper *callHelper, const char *datasource) {
    this->datasource = new char[strlen(datasource)];
    //字符串拷贝,防止java中的datasource的内存被释放
    strcpy(this->datasource, datasource);
    this->callHelper = callHelper;

}

DNFFmpeg::~DNFFmpeg() {
    DELETE(datasource);
    DELETE(callHelper);
    DELETE(audioChannel);
    DELETE(videoChannel);
    datasource = 0;

}

void DNFFmpeg::prepare() {
    //第一个参数pid
    //第二个参数线程属性传0即可
    //第三个参数,线程需要执行的方法,后面的参数,方法传入的参数
    pthread_create(&pid, 0, task_prepare, this);
}

/**
 * 创建这个方法能很方便的调用其内部的私有对象
 */
void DNFFmpeg::_prepare() {
    //打开网络
    avformat_network_init();
    //代表一个视频/音频包含了视频,音频的各种信息
    avFormatContext = avformat_alloc_context();
    AVDictionary *opts = NULL;
    //设置5秒超时
    av_dict_set(&opts, "timeout", "5000000", 0);

    //双重指针的意义在于可以更改指针的指向
    //1.打开音视频
    int ret = avformat_open_input(&avFormatContext, datasource, NULL, &opts);
    LOGE("%s open %d  %s", datasource, ret, av_err2str(ret));
    //非0就是打开视频失败
//    if (ret) {
    if (ret != 0) {
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }

    //2.查找音视频流
    ret = avformat_find_stream_info(avFormatContext, 0);
    //非0则说明查找失败
    if (ret < 0) {
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        return;
    }

    duration = avFormatContext->duration / 1000000;

    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        //可能代表一个视频也可能代表一个音频
        AVStream *avStream = avFormatContext->streams[i];
        //包含了解码这段视频流的各种参数信息
        AVCodecParameters *avCodecParameters = avStream->codecpar;
        //无论视频还是音频都需要干的一些事情（获得解码器）
        //a.通过当前流使用的编码方式查找解码器
        AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
        //找不到解码器回调
        if (avCodec == nullptr) {
            callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        //b.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(avCodec);
        if (context == nullptr) {
            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        //c.配置参数
        ret = avcodec_parameters_to_context(context, avCodecParameters);
        if (ret < 0) {
            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }

        if (avcodec_open2(context, avCodec, 0) != 0) {
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
        }
        //时间基
        AVRational base = avFormatContext->streams[i]->time_base;


        if (avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel;
        } else if (avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            //视频的帧率
            int fps = av_q2d(avFormatContext->streams[i]->avg_frame_rate);
            videoChannel = new VideoChannel(i, context, base, fps);
        }

    }
    //同时没有音频和视频,非媒体文件直接报错
    if (audioChannel == nullptr && videoChannel == nullptr) {
        callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }


}