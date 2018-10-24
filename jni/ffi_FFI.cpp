#include "jni/ffi_FFI.h"

#include <measurement_kit/ffi.h>

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_FFI_NettestStart(JNIEnv *env, jclass, jstring str) {
    jlong rv = 0;
    const char *s = env->GetStringUTFChars(str, nullptr);
    if (s != nullptr) {
        rv = (jlong)mk_nettest_start(s);
        env->ReleaseStringUTFChars(str, s);
    }
    return rv;
}

JNIEXPORT jboolean JNICALL
Java_io_ooni_mk_FFI_TaskIsDone(JNIEnv *, jclass, jlong nt) {
    return mk_task_is_done((mk_task_t *)nt) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_FFI_TaskWaitForNextEvent(JNIEnv *, jclass, jlong nt) {
    return (jlong)mk_task_wait_for_next_event((mk_task_t *)nt);
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_FFI_TaskInterrupt(JNIEnv *, jclass, jlong nt) {
    mk_task_interrupt((mk_task_t *)nt);
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_FFI_TaskDestroy(JNIEnv *, jclass, jlong nt) {
    mk_task_destroy((mk_task_t *)nt);
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_FFI_EventSerialize(JNIEnv *env, jclass, jlong ev) {
    jstring rv = nullptr;
    const char *s = mk_event_serialize((mk_event_t *)ev);
    if (s != nullptr) {
        rv = env->NewStringUTF(s);
    }
    return rv;
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_FFI_EventDestroy(JNIEnv *, jclass, jlong ev) {
    mk_event_destroy((mk_event_t *)ev);
}
