//
// Created by Administrator on 2019/7/2.
//

#include <pthread.h>
#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include <libavutil/imgutils.h>
}

/**
 * 丢包的方法
 * @param q
 */
void dropAvPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //如果不属于I帧 丢掉
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::realseAvPacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 丢解码后的视频帧 就不用管I帧和P帧
 * @param q
 */
void dropAvFrame(queue<AVFrame *> &q) {
    while (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::realseAvFrame(&frame);
        q.pop();

    }
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

VideoChannel::VideoChannel(int i, AVCodecContext *context, AVRational time_base, int fps)
        : BaseChannel(i, context, time_base) {
    avFrames.setReleaseCallback(realseAvFrame);
    this->fps = fps;
//    packages.setSyncHandle(dropAvPacket);
//    packages.sync();

    avFrames.setSyncHandle(dropAvFrame);
    avFrames.sync();
}


void VideoChannel::play() {
    LOGE("%s", "VideoChannel::play");
    isPlaying = 1;
    packages.setWork(1);
    avFrames.setWork(1);
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
    //每个画面刷新的间隔
    double frameDelays = 1.0 / fps;
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
        //获得 当前画面的播放的相对时间
        double videoClock = avFrame->best_effort_timestamp * av_q2d(time_base);
        //额外的间隔时间
        double extra_delay = avFrame->repeat_pict / (2 * fps);
        double delays = extra_delay + frameDelays;
        if (!audioChannel) {
            av_usleep(frameDelays * 1000 * 1000);
        } else {
            if (videoClock == 0) {
                av_usleep(frameDelays * 1000 * 1000);
            } else {
                //比较音频与视频
                double audioClock = audioChannel->clock;
                //音视频相差的间隔
                LOGE("视频时间:%f", videoClock);
                LOGE("音频时间:%f", audioClock);
                double diff = videoClock - audioClock;
                if (diff > 0) {
                    LOGE("视频快了%1f", diff);
                    //视频比较快
                    av_usleep((delays + diff) * 1000 * 1000);
                } else if (diff < 0) {
                    LOGE("音频快了%1f", diff);
                    //不睡了 让视频赶上音频
                    //视频包积压的太多了 (丢包)
                    if (fabs(diff) >= 0.05) {
                        //丢解码后的包
                        realseAvFrame(&avFrame);
                        avFrames.sync();
                        continue;
                    } else {
                        //在一个允许的范围之内
                    }
                }
            }
        }


        //回调出去播放
        //每次播放的时候做一次间隔
//        av_usleep(frameDelays * 1000 * 1000);
        callBack(dst[0], dstStride[0], context->width, context->height);
        realseAvFrame(&avFrame);
    }
    av_free(&dst[0]);
    realseAvFrame(&avFrame);

    //循环完成也就是播放完成了,那么就释放context
    isPlaying = 0;
    sws_freeContext(swsContext);
    swsContext = 0;

}


VideoChannel::~VideoChannel() {
    avFrames.clear();
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel = audioChannel;

}

void VideoChannel::stop() {
    isPlaying = 0;
    packages.setWork(0);
    packages.setWork(0);
    pthread_join(pid_decode, 0);
    pthread_join(pid_render, 0);
}

//公有的不需要这种方法
//void VideoChannel::setRenderFrame(RenderFrameCallBack callBack) {
//    this->callBack = callBack;
//}
