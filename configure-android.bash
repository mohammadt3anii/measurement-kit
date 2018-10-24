#!/bin/bash
set -e

dep_curl_version="7.61.1-8"
echo "* curl version: $dep_curl_version"

dep_geoip_api_c_version="1.6.12-8"
echo "* geoip_api_c version: $dep_geoip_api_c_version"

dep_libevent_version="2.1.8-8"
echo "* libevent version: $dep_libevent_version"

dep_libmaxminddb_version="1.3.2-8"
echo "* libmaxminddb version: $dep_libmaxminddb_version"

dep_libressl_version="2.7.4-8"
echo "* libressl version: $dep_libressl_version"

echo "* fetch prebuilt dependencies from github.com/measurement-kit/prebuilt"
rm -rf MK_DIST
./script/build/unix/install android-curl-$dep_curl_version                  \
                            android-geoip-api-c-$dep_geoip_api_c_version    \
                            android-libevent-$dep_libevent_version          \
                            android-libmaxminddb-$dep_libmaxminddb_version  \
                            android-libressl-$dep_libressl_version

echo "* Generate JNI headers using javah"
javac -cp java java/io/ooni/mk/ffi/FFI.java
javah -cp java -o jni/ffi_FFI.h io.ooni.mk.ffi.FFI

sed -e "s/@CURL_VERSION@/$dep_curl_version/g"                 \
    -e "s/@GEOIP_API_C_VERSION@/$dep_geoip_api_c_version/g"   \
    -e "s/@LIBEVENT_VERSION@/$dep_libevent_version/g"         \
    -e "s/@LIBMAXMINDDB_VERSION@/$dep_libmaxminddb_version/g" \
    -e "s/@LIBRESSL_VERSION@/$dep_libressl_version/g"         \
    jni/Android.mk.in > jni/Android.mk

echo "* Done. To continue run \`ndk-build\`."
