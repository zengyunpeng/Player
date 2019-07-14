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

    void start();

    void stop();

    void _start();

    void setRenderFrameCallBack(RenderFrameCallBack callBack);

    char *datasource = NULL;
    pthread_t pid;
    pthread_t player_pid;
    pthread_t stop_pid;
    AVFormatContext *avFormatContext = NULL;
    JavaCallHelper *callHelper = NULL;
    //申明一个指针一定要初始化，不然会有默认值
    AudioChannel *audioChannel = NULL;
    VideoChannel *videoChannel = NULL;
    int isPalying = 0;
    RenderFrameCallBack callBack;


};


#endif //PLAYER_DNFFMPEG_H
