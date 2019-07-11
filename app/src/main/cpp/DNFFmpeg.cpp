//
// Created by Administrator on 2019/6/26.
//
#include <cstring>
#include <pthread.h>
#include "DNFFmpeg.h"
#include "macro.h"


void *task_prepare(void *args) {
    DNFFmpeg *fFmpeg = static_cast<DNFFmpeg *>(args);
    fFmpeg->_prepare();
    return 0;
}

DNFFmpeg::DNFFmpeg(JavaCallHelper *callHelper, const char *datasource) {
    //strlen获得字符串的长度不包括\0，所以要加上1
    this->datasource = new char[strlen(datasource) + 1];
    //字符串拷贝,防止java中的datasource的内存被释放
    strcpy(this->datasource, datasource);
    this->callHelper = callHelper;

}

DNFFmpeg::~DNFFmpeg() {
    DELETE(datasource);
    DELETE(callHelper);
    if (audioChannel == NULL) {
        DELETE(audioChannel)
    }
    if (videoChannel == NULL) {
        DELETE(videoChannel)
    }

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
    avFormatContext = 0;
    //双重指针的意义在于可以更改指针的指向
    //1.打开音视频
    int ret = avformat_open_input(&avFormatContext, datasource, 0, 0);
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

    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        //可能代表一个视频也可能代表一个音频
        AVStream *stream = avFormatContext->streams[i];
        //包含了解码这段视频流的各种参数信息
        AVCodecParameters *codecpar = stream->codecpar;
        //无论视频还是音频都需要干的一些事情（获得解码器）
        //a.通过当前流使用的编码方式查找解码器
        AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        //找不到解码器回调
        if (codec == NULL) {
            LOGE("查找解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
        }
        //b.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(codec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        //c.设置上下文的一些参数
        ret = avcodec_parameters_to_context(context, codecpar);
        if (ret < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
        }

        //d.打开编码器
        ret = avcodec_open2(context, codec, 0);
        if (ret < 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
        }

        //4.打开编码器
        ret = avcodec_open2(context, codec, 0);

        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, context);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoChannel = new VideoChannel(i, context);
            videoChannel->callBack = this->callBack;
        }
    }

    //不是媒体文件(很少见)
    if (audioChannel == NULL && videoChannel == NULL) {
        LOGE("打开解码器失败:%s", "1");
        callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
    }

    //准备完了，通知java
    callHelper->onPrepare(THREAD_CHILD);


}

void *play(void *args) {
    LOGE("%s", "void *play(void *args) {");
    DNFFmpeg *dnfFmpeg = static_cast<DNFFmpeg *>(args);
    dnfFmpeg->_start();
    //必须返回0
    return 0;
}

void DNFFmpeg::start() {
    //正在播放的标记
    isPalying = 1;
    if (videoChannel) {
        LOGE("开始进行视频流的播放");
        videoChannel->packages.setWork(1);
        videoChannel->avFrames.setWork(1);
        videoChannel->play();
    }

    if (audioChannel) {
        audioChannel->play();
    }
    pthread_create(&player_pid, 0, play, this);

}

/**
 * 专门读取数据包
 */
void DNFFmpeg::_start() {
    //正在播放的标记
    int ret;
    while (isPalying) {
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(avFormatContext, packet);
        if (ret == 0) {
            //stream_index这个流的一个序号
            if (audioChannel && packet->stream_index == audioChannel->id) {
                //音频

                audioChannel->packages.push(packet);

            } else if (videoChannel && packet->stream_index == videoChannel->id) {
                //视频
                videoChannel->packages.push(packet);
            }

        } else if (ret == AVERROR_EOF) {
            //读取完成，但是有可能还没播放完

        } else {

        }
    }


}

void DNFFmpeg::setRenderFrameCallBack(RenderFrameCallBack callBack) {
    this->callBack = callBack;
//    if (videoChannel) {
//        videoChannel->callBack = callBack;
//    }
//    if (audioChannel) {
////        audioChannel
//    }
}
