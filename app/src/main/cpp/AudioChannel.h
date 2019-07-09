//
// Created by Administrator on 2019/7/2.
//

#ifndef PLAYER_AUDIOCHANNEL_H
#define PLAYER_AUDIOCHANNEL_H

#include "BaseChannel.h"
class AudioChannel :public BaseChannel{
public:
    AudioChannel(int i,AVCodecContext *context);

private:
    void play();
};


#endif //PLAYER_AUDIOCHANNEL_H
