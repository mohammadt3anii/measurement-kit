#include "jni/vendor_mkcurl_FFI.h"

#include <measurement_kit/vendor/measurement-kit/mkcurl.h>

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestNew(JNIEnv *, jclass) {
  return (jlong)mkcurl_request_new();
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetCABundlePath(
        JNIEnv *env, jclass, jlong handle, jstring path) {
  if (env != nullptr && handle != 0 && path != nullptr) {
    const char *s = env->GetStringUTFChars(path, nullptr);
    if (s != nullptr) {
      mkcurl_request_set_ca_bundle_path((mkcurl_request_t *)handle, s);
      env->ReleaseStringUTFChars(path, s);
    }
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestEnableHTTP2(
    JNIEnv *, jclass, jlong handle) {
  if (handle != 0) mkcurl_request_enable_http2((mkcurl_request_t *)handle);
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetMethodPOST(
    JNIEnv *, jclass, jlong handle) {
  if (handle != 0) mkcurl_request_set_method_post((mkcurl_request_t *)handle);
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetURL(
    JNIEnv *env, jclass, jlong handle, jstring url) {
  if (env != nullptr && handle != 0 && url != nullptr) {
    const char *s = env->GetStringUTFChars(url, nullptr);
    if (s != nullptr) {
      mkcurl_request_set_url((mkcurl_request_t *)handle, s);
      env->ReleaseStringUTFChars(url, s);
    }
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestAddHeader(
    JNIEnv *env, jclass, jlong handle, jstring header) {
  if (env != nullptr && handle != 0 && header != nullptr) {
    const char *s = env->GetStringUTFChars(header, nullptr);
    if (s != nullptr) {
      mkcurl_request_add_header((mkcurl_request_t *)handle, s);
      env->ReleaseStringUTFChars(header, s);
    }
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetBody(
    JNIEnv *env, jclass, jlong handle, jbyteArray body) {
  // Note: jsize is int32_t for both Java and Android
  if (env != nullptr && handle != 0 && body != nullptr) {
    jsize count = env->GetArrayLength(body);
    if (count < 0) return;
    jbyte *base = env->GetByteArrayElements(body, nullptr);
    if (base == nullptr) return;
    mkcurl_request_set_body_binary_v2((mkcurl_request_t *)handle,
                                      (const uint8_t *)base, (size_t)count);
    env->ReleaseByteArrayElements(body, base, 0);
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetTimeout(
    JNIEnv *, jclass, jlong handle, jlong timeout) {
  if (handle != 0) {
    mkcurl_request_set_timeout((mkcurl_request_t *)handle, timeout);
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestSetProxyURL(
    JNIEnv *env, jclass, jlong handle, jstring url) {
  if (env != nullptr && handle != 0 && url != nullptr) {
    const char *s = env->GetStringUTFChars(url, nullptr);
    if (s != nullptr) {
      mkcurl_request_set_proxy_url((mkcurl_request_t *)handle, s);
      env->ReleaseStringUTFChars(url, s);
    }
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestEnableFollowRedirect(
    JNIEnv *, jclass, jlong handle) {
  if (handle != 0) {
    mkcurl_request_enable_follow_redirect((mkcurl_request_t *)handle);
  }
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_RequestDelete(
    JNIEnv *, jclass, jlong handle) {
  mkcurl_request_delete((mkcurl_request_t *)handle);
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetError(
    JNIEnv *, jclass, jlong handle) {
  // Note: mkcurl_response_get_*() handles nullptr gracefully
  return mkcurl_response_get_error((mkcurl_response_t *)handle);
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetRedirectURL(
    JNIEnv *env, jclass, jlong handle) {
  jstring rv = nullptr;
  if (env != nullptr && handle != 0) {
    const char *s = mkcurl_response_get_redirect_url(
        (mkcurl_response_t *)handle);
    if (s != nullptr) {
      rv = env->NewStringUTF(s);
    }
  }
  return rv;
}

JNIEXPORT jbyteArray JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetBody(
    JNIEnv *env, jclass, jlong handle) {
  jbyteArray rv = nullptr;
  if (env != nullptr && handle != 0) {
    const uint8_t *base = nullptr;
    size_t count = 0;
    if (mkcurl_response_get_body_binary_v2((mkcurl_response_t *)handle,
                                           &base, &count) == true &&
        base != nullptr && count <= INT_MAX) {
      rv = env->NewByteArray((jsize)count);
      if (rv != nullptr) {
        env->SetByteArrayRegion(rv, 0, (jsize)count, (const jbyte *)base);
      }
    }
  }
  return rv;
}

JNIEXPORT jdouble JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetBytesSent(
    JNIEnv *, jclass, jlong handle) {
  // Note: mkcurl_response_get_*() handles nullptr gracefully
  return mkcurl_response_get_bytes_sent((mkcurl_response_t *)handle);
}

JNIEXPORT jdouble JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetBytesRecv(
    JNIEnv *, jclass, jlong handle) {
  // Note: mkcurl_response_get_*() handles nullptr gracefully
  return mkcurl_response_get_bytes_recv((mkcurl_response_t *)handle);
}

JNIEXPORT jbyteArray JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetLogs(
    JNIEnv *env, jclass, jlong handle) {
  jbyteArray rv = nullptr;
  if (env != nullptr && handle != 0) {
    const uint8_t *base = nullptr;
    size_t count = 0;
    if (mkcurl_response_get_logs_binary_v2((mkcurl_response_t *)handle,
                                           &base, &count) == true &&
        base != nullptr && count <= INT_MAX) {
      rv = env->NewByteArray((jsize)count);
      if (rv != nullptr) {
        env->SetByteArrayRegion(rv, 0, (jsize)count, (const jbyte *)base);
      }
    }
  }
  return rv;
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetRequestHeaders(
    JNIEnv *env, jclass, jlong handle) {
  jstring rv = nullptr;
  if (env != nullptr && handle != 0) {
    const char *s = mkcurl_response_get_request_headers(
        (mkcurl_response_t *)handle);
    if (s != nullptr) {
      rv = env->NewStringUTF(s);
    }
  }
  return rv;
}

JNIEXPORT jbyteArray JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetResponseHeaders(
    JNIEnv *env, jclass, jlong handle) {
  jbyteArray rv = nullptr;
  if (env != nullptr && handle != 0) {
    const uint8_t *base = nullptr;
    size_t count = 0;
    if (mkcurl_response_get_response_headers_binary_v2(
            (mkcurl_response_t *)handle, &base, &count) == true &&
        base != nullptr && count <= INT_MAX) {
      rv = env->NewByteArray((jsize)count);
      if (rv != nullptr) {
        env->SetByteArrayRegion(rv, 0, (jsize)count, (const jbyte *)base);
      }
    }
  }
  return rv;
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseGetCertificateChain(
    JNIEnv *env, jclass, jlong handle) {
  jstring rv = nullptr;
  if (env != nullptr && handle != 0) {
    const char *s = mkcurl_response_get_certificate_chain(
        (mkcurl_response_t *)handle);
    if (s != nullptr) {
      rv = env->NewStringUTF(s);
    }
  }
  return rv;
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_ResponseDelete(
    JNIEnv *, jclass, jlong handle) {
  mkcurl_response_delete((mkcurl_response_t *) handle);
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkcurl_FFI_Perform(JNIEnv *, jclass, jlong handle) {
  // Note: mkcurl_perform handles nullptr gracefully
  return (jlong)mkcurl_perform((mkcurl_request_t *)handle);
}
