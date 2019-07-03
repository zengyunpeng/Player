//
// Created by Administrator on 2019/6/26.
//

#ifndef PLAYER_DNFFMPEG_H
#define PLAYER_DNFFMPEG_H


#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C" {
#include "include/libavformat/avformat.h"
}

class DNFFmpeg {
public:
    DNFFmpeg(JavaCallHelper *callHelper, const char *datasource);

    ~DNFFmpeg();

    void prepare();

    void _prepare();

private:
    char *datasource;
    pthread_t pid;
    AVFormatContext *avFormatContext;
    JavaCallHelper *callHelper;
    AudioChannel *audioChannel;
    VideoChannel *videoChannel;
    int64_t duration;
};


#endif //PLAYER_DNFFMPEG_H
