//
// Created by Administrator on 2019/7/2.
//

#ifndef PLAYER_VIDEOCHANNEL_H
#define PLAYER_VIDEOCHANNEL_H

#include "BaseChannel.h"

extern "C" {
#include <libswscale/swscale.h>
};

//定义一种回调类型
typedef void (*RenderFrameCallBack)(uint8_t *, int, int, int);


class VideoChannel : public BaseChannel {
public:
    VideoChannel(int i, AVCodecContext *context);

    ~VideoChannel();

    void play();

    void decode();

    void render();

    void setRenderFrame(RenderFrameCallBack callBack);

    pthread_t pid_decode;
    pthread_t pid_render;
    RenderFrameCallBack callBack;


};


#endif //PLAYER_VIDEOCHANNEL_H
