//
// Created by Administrator on 2019/7/2.
//

#include <pthread.h>
#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include <libavutil/imgutils.h>
}

void *decode_task(void *args) {
    LOGE("开始进行视频的解码%s", "");
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decode();
    return 0;
}

void *render_task(void *args) {
    LOGE("开始进行视频的渲染%s", "");
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->render();
    return 0;
}

VideoChannel::VideoChannel(int i, AVCodecContext *context) : BaseChannel(i, context) {
    avFrames.setReleaseCallback(realseAvFrame);
}

void VideoChannel::play() {
    LOGE("%s", "VideoChannel::play");
    isPlaying = 1;
    //解码
    pthread_create(&pid_decode, 0, decode_task, this);
    //播放
    pthread_create(&pid_render, 0, render_task, this);

}


void VideoChannel::decode() {
    AVPacket *avPacket = 0;
    while (isPlaying) {
        int ret = packages.pop(avPacket);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        //把包丢给解码器
        ret = avcodec_send_packet(context, avPacket);
        realseAvPacket(&avPacket);
        if (ret != 0) {
            break;
        }
        //代表了一个图像
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取解码后的数据包
        ret = avcodec_receive_frame(context, frame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {
            break;
        }

        //再开一个线程去播放，这样就不会影响解码
        avFrames.push(frame);
    }

    //释放
    realseAvPacket(&avPacket);

}

/**
 * 播放/渲染的方法
 */
void VideoChannel::render() {
    //前三个参数原图的 后三个目标的属性,后面的编解码的属性
    LOGE("void VideoChannel::render() {%s", "");
    SwsContext *swsContext = sws_getContext(
            context->width, context->height, context->pix_fmt,
            context->width, context->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);
    AVFrame *avFrame = 0;
    //指针数组
    uint8_t *dst[4];
    int dstStride[4];
    //申请内存空间
    av_image_alloc(dst, dstStride,
                   context->width, context->height, AV_PIX_FMT_RGBA, 1);
    LOGE("isPlaying: %d", isPlaying);
    while (isPlaying) {
        int ret = avFrames.pop(avFrame);
        if (!isPlaying) {
            LOGE("break");
            break;
        }
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(avFrame->data),
                  avFrame->linesize, 0,
                  context->height,
                  dst,
                  dstStride);
        //回调出去播放

        callBack(dst[0], dstStride[0], context->width, context->height);
        realseAvFrame(&avFrame);
    }
    av_free(&dst[0]);
    realseAvFrame(&avFrame);
}

VideoChannel::~VideoChannel() {
    avFrames.clear();
}

//公有的不需要这种方法
//void VideoChannel::setRenderFrame(RenderFrameCallBack callBack) {
//    this->callBack = callBack;
//}
