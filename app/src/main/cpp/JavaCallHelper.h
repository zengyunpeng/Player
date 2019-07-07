//
// Created by Administrator on 2019/6/26.
//

#ifndef PLAYER_JAVACALLHELPER_H
#define PLAYER_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    //第一个javavm ，用于切换县城
    //第二个参数 当前线程的jnienv
    //第三个参数 Java中的DNPlayer对象
    JavaCallHelper(JavaVM *vm, JNIEnv *jnienv, jobject instance);

    ~JavaCallHelper();

    //回调java
    void onError(int thread, int error);


    void onPrepare(int thread);

private:
    JavaVM *vm;
    JNIEnv *jnienv;
    jobject instance;
    jmethodID onErrorId;
    jmethodID onPrepareId;
};


#endif //PLAYER_JAVACALLHELPER_H
