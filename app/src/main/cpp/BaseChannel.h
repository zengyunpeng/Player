//
// Created by Administrator on 2019/7/7.
//

#ifndef PLAYER_BASECHANNEL_H
#define PLAYER_BASECHANNEL_H


#include "safe_queue.h"

extern "C" {

#include <libavcodec/avcodec.h>
};


class BaseChannel {
public:
    BaseChannel(int id, AVCodecContext *context, AVRational time_base) : id(id), context(context),
                                                                         time_base(time_base) {
        packages.setReleaseCallback(BaseChannel::realseAvPacket);
        avFrames.setReleaseCallback(BaseChannel::realseAvFrame);
    };

    //申明virtual ,不声明会只调用父类不调用子类的析构方法
    virtual ~BaseChannel() {
        packages.clear();
        avFrames.clear();

    };

    static void realseAvPacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            *packet = 0;
        }

    }


    static void realseAvFrame(AVFrame **avFrame) {
        if (avFrame) {
            av_frame_free(avFrame);
            *avFrame = 0;
        }

    }

    //解码，播放操作
    virtual void play() = 0;

    int id;
    AVCodecContext *context;
    SafeQueue<AVPacket *> packages;
    int isPlaying;
    SafeQueue<AVFrame *> avFrames;
    AVRational time_base;
};


#endif //PLAYER_BASECHANNEL_H
