//
// Created by Administrator on 2019/7/2.
//

#include "AudioChannel.h"

void *audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

void *audio_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return 0;
}


void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //获得pcm 数据 多少个字节 data
    int dataSize = audioChannel->getPcm();
    if (dataSize > 0) {
        (*bq)->Enqueue(bq, audioChannel->data, dataSize);
    }
}

AudioChannel::AudioChannel(int i, AVCodecContext *context, AVRational time_base) : BaseChannel(i,
                                                                                               context,
                                                                                               time_base) {
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    //44100个16位 44100 * 2
    // 44100*(双声道)*(16位)
    data = static_cast<uint8_t *>(malloc(out_sample_rate * out_channels * out_samplesize));
    memset(data, 0, out_sample_rate * out_channels * out_samplesize);

}


AudioChannel::~AudioChannel() {
    if (data) {
        free(data);
        data = 0;
    }
}


void AudioChannel::play() {
    //设置为播放状态
    packages.setWork(1);
    avFrames.setWork(1);
    //0+输出声道+输出采样位+输出采样率+  输入的3个参数
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, out_sample_rate,
                                    context->channel_layout, context->sample_fmt,
                                    context->sample_rate, 0, 0);
    swr_init(swrContext);
    isPlaying = 1;
    pthread_create(&pid_audio_decode, 0, audio_decode, this);

    pthread_create(&pid_audio_play, 0, audio_play, this);

}

void AudioChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        LOGE("音频包的长度: %d", packages.size());
        int ret = packages.pop(packet);
        LOGE("去除音频包的结果:%d ", ret);
        if (!isPlaying) {
            return;
        }
        if (!ret) {
            continue;
        }
        //把包丟給解碼器
//        av_read_frame()
        ret = avcodec_send_packet(context, packet);
        //这里会有一个AVERROR_INVALIDDATA = -1094995529的错误
        //-12
        //-1094995529        Invalid data found when processing input
        LOGE("音频接包结果: %d", ret);
        LOGE("音频接包结果: %s", av_err2str(ret));
        realseAvPacket(&packet);
        if (ret != 0) {
            break;
        }
        //代表一个图像
        AVFrame *frame = av_frame_alloc();
        //从解码器中读取 解码后的数据包 AVFrame
        ret = avcodec_receive_frame(context, frame);
        LOGE("音频包转为帧结果: %d", ret);
        if (ret == AVERROR(EAGAIN)) {
            //需要更多的数据才能解码
            continue;
        } else if (ret != 0) {
            break;
        }
        //新开一个线程来播放(提高流畅度)
        LOGE("往avFrames里面添加数据");
        avFrames.push(frame);
    }
    realseAvPacket(&packet);
}


void AudioChannel::_play() {
    /**
     * 1.创建引擎并获取引擎接口
     */
    SLresult result;
    //1.1 创建引擎SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //1.2 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //1.3 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    /**
     * 2.设置混音器
     */
    //2.1 创建混音器
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //2.2 初始化混音器
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        return;
    }

    /**
     * 3. 创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    //pcm+2(双声道)+44100(采样率)+ 16(采样位)+16(数据的大小)+LEFT|RIGHT(双声道)+小端数据
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};
    //3.2 配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口 操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到播放器后调用 获取player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);

    /**
     * 4. 设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueInterface);
    //这里bqPlayerBufferQueueInterface为null
    (*bqPlayerBufferQueueInterface)->RegisterCallback(bqPlayerBufferQueueInterface,
                                                      bqPlayerCallback, this);
    /**
    * 5、设置播放状态
    */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);

    /**
   * 6、手动激活一下这个回调
   */
    bqPlayerCallback(bqPlayerBufferQueueInterface, this);

}

int AudioChannel::getPcm() {
    int data_size = 0;
    AVFrame *frame;
    LOGE("avFrames%d", avFrames.size());
    int ret = avFrames.pop(frame);
    LOGE("&frame%p", &frame);
    if (!isPlaying) {
        if (ret) {
            realseAvFrame(&frame);
        }
        return data_size;
    }
    //48000HZ 8位 =》 44100 16位
    //重采样
    // 假设我们输入了10个数据 ，swrContext转码器 这一次处理了8个数据
    // 那么如果不加delays(上次没处理完的数据) , 积压
    int64_t delays = swr_get_delay(swrContext, frame->sample_rate);
    // 将 nb_samples 个数据 由 sample_rate采样率转成 44100 后 返回多少个数据
    // 10  个 48000 = nb 个 44100
    // AV_ROUND_UP : 向上取整 1.1 = 2
    int64_t max_samples = av_rescale_rnd(delays + frame->nb_samples,
                                         out_sample_rate, frame->sample_rate, AV_ROUND_UP);
    //上下文+输出缓冲区+输出缓冲区能接受的最大数据量+输入数据+输入数据个数
    //返回 每一个声道的输出数据
    int samples = swr_convert(swrContext, &data, max_samples, (const uint8_t **) frame->data,
                              frame->nb_samples);

    data_size = samples * out_samplesize * out_channels;
    //获取frame的一个相对播放时间
    //获得相对播放这一段数据的秒数
    clock = frame->pts * av_q2d(time_base);
    realseAvFrame(&frame);
    return data_size;
}

void AudioChannel::stop() {
    isPlaying = 0;
    packages.setWork(0);
    avFrames.setWork(0);
    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_play, 0);
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = 0;
    }
    //释放播放器
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = nullptr;
        bqPlayerInterface = nullptr;
        bqPlayerBufferQueueInterface = nullptr;
    }
    //释放混音器
    if (outputMixObject) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
    }

    //釋放引擎
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineInterface = nullptr;
    }


};
