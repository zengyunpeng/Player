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
    //子线程中调用 会调用成员变量在主线程中的析构方法会出现问题
//    DELETE(callHelper);
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
    isPalying = 1;
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
    //设定一个超时时间
    AVDictionary *option = 0;
    av_dict_set(&option, "timeout", "5000000", 0);
    //这实际上是一个网络请求,打开socket去链接数据源
    int ret = avformat_open_input(&avFormatContext, datasource, 0, &option);
    av_dict_free(&option);
    //非0就是打开视频失败
//    if (ret) {
    if (ret != 0) {
        if (isPalying) {
            isPalying = 0;
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }

    //2.查找音视频流
    ret = avformat_find_stream_info(avFormatContext, 0);
    //非0则说明查找失败
    if (ret < 0) {
        if (isPalying) {
            isPalying = 0;
            callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
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
            if (isPalying) {
                isPalying = 0;
                callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
        }
        //b.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(codec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(ret));
            if (isPalying) {
                isPalying = 0;
                callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }
        //c.设置上下文的一些参数
        ret = avcodec_parameters_to_context(context, codecpar);
        if (ret < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(ret));
            if (isPalying) {
                isPalying = 0;
                callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //d.打开编码器
        ret = avcodec_open2(context, codec, 0);
        if (ret < 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            if (isPalying) {
                isPalying = 0;
                callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }

        //4.打开编码器
//        ret = avcodec_open2(context, codec, 0);
        AVRational tme_base = stream->time_base;

        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, context, tme_base);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //帧率
            AVRational avRational = stream->avg_frame_rate;
            int fps = av_q2d(avRational);
            videoChannel = new VideoChannel(i, context, tme_base, fps);
            videoChannel->callBack = this->callBack;
        }
    }

    //不是媒体文件(很少见)
    if (audioChannel == NULL && videoChannel == NULL) {
        LOGE("打开解码器失败:%s", "1");
        if (isPalying) {
            isPalying = 0;
            callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    //准备完了，通知java
    if (isPalying) {
        callHelper->onPrepare(THREAD_CHILD);
    }
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


    if (audioChannel) {
        audioChannel->play();
    }

    if (videoChannel) {
        LOGE("开始进行视频流的播放");
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }
    //读取音视频包
    pthread_create(&player_pid, 0, play, this);

}

/**
 * 专门读取数据包
 */
void DNFFmpeg::_start() {
    //正在播放的标记
    int ret;
    int videoTime = 0;
    int audioTime = 0;

    while (isPalying) {
        //音频包的个数大于视频包
//        LOGETag("视频包个数: %d", videoTime);
//        LOGETag("音频包个数: %d", audioTime);
        //如果包太多了就等待,就等待防止oom
        //特别是读本地文件的时候 读取速度非常快,更要避免这个问题
        //这里会存在packages一直大于100的情况,然后卡住所有线程
//        LOGE("audioChannel->packages: %d", audioChannel->packages.size());
//        LOGE("audioChannel && audioChannel->packages.size() > 100: %d",
//             audioChannel && audioChannel->packages.size() > 100);
        if (audioChannel && audioChannel->packages.size() > 100) {
            //睡10毫秒
            av_usleep(1000 * 10);
            continue;
        }
//        LOGE("videoChannel->packages: %d", videoChannel->packages.size());
//        LOGE("videoChannel && videoChannel->packages.size() > 100: %d",
//             videoChannel && videoChannel->packages.size() > 100);
        //这里会存在packages一直大于100的情况,然后卡住所有线程
        if (videoChannel && videoChannel->packages.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }

        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(avFormatContext, packet);
//        LOGE("av_read_frame结果:%d", ret);
        if (ret == 0) {
            //stream_index这个流的一个序号
//            LOGE("audioChannel && packet->stream_index == audioChannel->id结果:%d",
//                 audioChannel && packet->stream_index == audioChannel->id);
            if (audioChannel && packet->stream_index == audioChannel->id) {
                //音频
                audioTime++;
//                LOGE("往音频包里push数据");
                audioChannel->packages.push(packet);

            } else if (videoChannel && packet->stream_index == videoChannel->id) {
                //视频
                videoTime++;
                videoChannel->packages.push(packet);
            }

        } else if (ret == AVERROR_EOF) {
            //读取完成，但是有可能还没播放完
            LOGE("AVERROR_EOF");
            if (audioChannel->packages.empty() && audioChannel->avFrames.empty() &&
                videoChannel->packages.empty() && videoChannel->avFrames.empty()) {
                break;
            }
            //为什么这里是让他继续循环 而不是sleep
            //如果是做直播可以sleep
            //如果要支持点播(播放本地文件) 就有seek的情况

        } else {
            break;
        }
    }
    isPalying = 0;
    audioChannel->stop();
    videoChannel->stop();
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

void *async_stop(void *args) {
    DNFFmpeg *dnfFmpeg = static_cast<DNFFmpeg *>(args);
    //等待prepare结束
    //让准备线程等待
    pthread_join(dnfFmpeg->pid, 0);
    //保证start也等待
    pthread_join(dnfFmpeg->player_pid, 0);
    DELETE(dnfFmpeg->audioChannel);
    DELETE(dnfFmpeg->videoChannel);
    if (dnfFmpeg->avFormatContext) {
        //先关闭读取
        avformat_close_input(&dnfFmpeg->avFormatContext);

        avformat_free_context(dnfFmpeg->avFormatContext);

        dnfFmpeg->avFormatContext = 0;
    }
    //子线程中调用 会调用成员变量在主线程中的析构方法会出现问题
    DELETE(dnfFmpeg);
    dnfFmpeg = 0;
    return 0;
}

void DNFFmpeg::stop() {
    isPalying = 0;
    callHelper = 0;
    //卡住线程
    pthread_create(&stop_pid, 0, async_stop, this);

}
