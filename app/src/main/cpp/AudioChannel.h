//
// Created by Administrator on 2019/7/2.
//

#ifndef PLAYER_AUDIOCHANNEL_H
#define PLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel {
public:
    AudioChannel(int i, AVCodecContext *context);

    ~AudioChannel();

    void play();

    void decode();

    void _play();

    int getPcm();


    int out_channels;
    int out_samplesize;
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;
    //重采样
    SwrContext *swrContext = 0;
    /**
     * OpenSL ES
     */
    // 引擎与引擎接口
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;
    //播放器队列接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface = 0;


    int out_sample_rate;
    uint8_t *data = 0;


};


#endif //PLAYER_AUDIOCHANNEL_H
