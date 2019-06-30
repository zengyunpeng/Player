//
// Created by Administrator on 2019/6/26.
//

#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {
    //在子线程中就要用到JavaVM
    this->vm = vm;
    //如果在子线程回调,用jnienv
    this->jnienv = jnienv;
    //一旦涉及到jobject跨方法跨线程调用就需要创建全局引用
    this->instance = env->NewWeakGlobalRef(instance);

    jclass clazz = env->GetObjectClass(instance);
    onErrorId = env->GetMethodID(clazz, "onError", "(I)V");

}

JavaCallHelper::~JavaCallHelper() {
    jnienv->DeleteGlobalRef(instance);
}

void JavaCallHelper::onError(int thread, int error) {
    if (thread == THREAD_MAIN) {
        //主线程直接回调
        jnienv->CallVoidMethod(instance, onErrorId, error);
    } else {
        //子线程
        JNIEnv *env;
        //获得当前线程jniEnv
        vm->AttachCurrentThread(&env, 0);
        env->CallVoidMethod(instance, onErrorId, vm);
        vm->DetachCurrentThread();

    }

}
