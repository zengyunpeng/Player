//
// Created by Administrator on 2019/7/3.
//

#ifndef PLAYER_VIDEOCHANNEL_H
#define PLAYER_VIDEOCHANNEL_H


#include <libavcodec/avcodec.h>

class VideoChannel {
public:
    VideoChannel(int i, AVCodecContext *context, AVRational avRational, int fps);

    ~VideoChannel();

private:
    int i;
    AVCodecContext *context;
    AVRational avRational;
    int fps;
};


#endif //PLAYER_VIDEOCHANNEL_H
