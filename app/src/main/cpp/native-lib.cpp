#include <jni.h>
#include <string>
#include "DNFFmpeg.h"

DNFFmpeg *dnFFmpeg = 0;

JavaVM *javaVM;

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

    dnFFmpeg->prepare();

    env->ReleaseStringUTFChars(datasource_, datasource);
}