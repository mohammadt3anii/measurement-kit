#include "jni/vendor_mkgeoip_FFI.h"

#include <measurement_kit/vendor/mkgeoip.h>

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_UbuntuGetURL(JNIEnv *env, jclass) {
  return (env != nullptr)
             ? env->NewStringUTF(mkgeoip_ubuntu_get_url())
             : nullptr;
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_UbuntuParserNew(JNIEnv *, jclass) {
  return (jlong)mkgeoip_ubuntu_parser_new();
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_UbuntuParserGetProbeIP(
    JNIEnv *env, jclass, jlong parser, jlong response) {
  if (env == nullptr || parser == 0 || response == 0) return nullptr;
  const char *s = mkgeoip_ubuntu_parser_get_probe_ip(
      (mkgeoip_ubuntu_parser_t *)parser,
      (const mkcurl_response_t *)response);
  return (s != nullptr) ? env->NewStringUTF(s) : nullptr;
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_UbuntuParserDelete(
    JNIEnv *, jclass, jlong handle) {
  mkgeoip_ubuntu_parser_delete((mkgeoip_ubuntu_parser_t *)handle);
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_MMDBOpen(JNIEnv *env, jclass, jstring path) {
  if (env == nullptr || path == nullptr) return 0;
  const char *s = env->GetStringUTFChars(path, nullptr);
  if (s == nullptr) return 0;
  mkgeoip_mmdb_t *mmdb = mkgeoip_mmdb_open(s);
  env->ReleaseStringUTFChars(path, s);
  return (jlong)mmdb;
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_MMDBLookupCC(
    JNIEnv *env, jclass, jlong handle, jstring ip) {
  if (env == nullptr || handle == 0 || ip == nullptr) return nullptr;
  const char *s = env->GetStringUTFChars(ip, nullptr);
  if (s == nullptr) return nullptr;
  const char *rv = mkgeoip_mmdb_lookup_cc((mkgeoip_mmdb_t *)handle, s);
  env->ReleaseStringUTFChars(ip, s);
  return (rv != nullptr) ? env->NewStringUTF(rv) : nullptr;
}

JNIEXPORT jlong JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_MMDBLookupASN(
    JNIEnv *env, jclass, jlong handle, jstring ip) {
  if (env == nullptr || handle == 0 || ip == nullptr) return 0;
  const char *s = env->GetStringUTFChars(ip, nullptr);
  if (s == nullptr) return 0;
  return mkgeoip_mmdb_lookup_asn((mkgeoip_mmdb_t *)handle, s);
}

JNIEXPORT jstring JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_MMDBLookupOrg(
    JNIEnv *env, jclass, jlong handle, jstring ip) {
  if (env == nullptr || handle == 0 || ip == nullptr) return nullptr;
  const char *s = env->GetStringUTFChars(ip, nullptr);
  if (s == nullptr) return nullptr;
  const char *rv = mkgeoip_mmdb_lookup_org((mkgeoip_mmdb_t *)handle, s);
  env->ReleaseStringUTFChars(ip, s);
  return (rv != nullptr) ? env->NewStringUTF(rv) : nullptr;
}

JNIEXPORT void JNICALL
Java_io_ooni_mk_vendor_mkgeoip_FFI_MMDBClose(
    JNIEnv *, jclass, jlong handle) {
  mkgeoip_mmdb_close((mkgeoip_mmdb_t *)handle);
}
