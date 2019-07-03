//
// Created by Administrator on 2019/7/3.
//

#include "VideoChannel.h"
#include "include/libavcodec/avcodec.h"
#include "macro.h"

VideoChannel::VideoChannel(int i, AVCodecContext *context, AVRational avRational, int fps) {
    this->i = i;
    this->context = context;
    this->avRational = avRational;
    this->fps = fps;
}

//VideoChannel::~VideoChannel() {
//    DELETE(i);
//    DELETE(context);
//    DELETE(avRational);
//    DELETE(fps);
//}
