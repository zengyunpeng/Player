#include <jni.h>
#include <string>
#include "DNFFmpeg.h"
//导入本地窗口
#include <android/native_window_jni.h>

DNFFmpeg *dnFFmpeg = 0;

JavaVM *javaVM = 0;
ANativeWindow *window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void render(uint8_t *data, int linesize, int width, int height) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        return;
    }
    //拷贝数据
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);

    //一行有多少个数据*4  一个数据包含RGBA，所以*4
    int dst_linesize = window_buffer.stride * 4;

    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst_data + i * dst_linesize, data + i * linesize, dst_linesize);
    }


    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

int JNI_OnLoad(JavaVM *vm, void *r) {
    javaVM = vm;
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_DNPlayer_native_1prepare(JNIEnv *env, jobject instance,
                                                 jstring datasource_) {
    const char *datasource = env->GetStringUTFChars(datasource_, 0);
    JavaCallHelper *javaCallHelper = new JavaCallHelper(javaVM, env, instance);
    //创建一个播放器
    dnFFmpeg = new DNFFmpeg(javaCallHelper, datasource);
    dnFFmpeg->setRenderFrameCallBack(render);
    dnFFmpeg->prepare();

    env->ReleaseStringUTFChars(datasource_, datasource);
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_DNPlayer_native_1start(JNIEnv *env, jobject instance) {

    dnFFmpeg->start();

}extern "C"
JNIEXPORT void JNICALL
Java_com_example_player_DNPlayer_native_1setSurface(JNIEnv *env, jobject instance,
                                                    jobject surface) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);

    }
    window = ANativeWindow_fromSurface(env, surface);
    //设置窗口属性

    pthread_mutex_unlock(&mutex);
}