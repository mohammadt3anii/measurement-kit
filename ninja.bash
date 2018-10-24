#!/bin/bash
set -e

# OS discovery and OS specific actions
# ------------------------------------

if [ "$PKG_OS" = "" ]; then
  PKG_OS=`uname -s | tr [A-Z] [a-z]`
  [ "$PKG_OS" != "darwin" ] || PKG_OS=macos
fi
echo "* PKG_OS: $PKG_OS"

if [ "$PKG_OS" = "macos" -o "$PKG_OS" = "ios" ]; then
  echo "* CPPFLAGS += -DHAVE_DECL_OPTRESET"
  CPPFLAGS="$CPPFLAGS -DHAVE_DECL_OPTRESET"
fi

# Import build variables
# ----------------------

PKG_ARCH=${PKG_ARCH:-`uname -m`}
echo "* PKG_ARCH: $PKG_ARCH"

AR=${AR:-ar}
echo "* AR: $AR"

CC=${CC:-gcc}
echo "* CC: $CC"

echo "* CFLAGS: $CFLAGS"

CXX=${CXX:-g++}
echo "* CXX: $CXX"

echo "* CXXFLAGS: $CXXFLAGS"

echo "* LDFLAGS: $LDFLAGS"

echo "* LIBS: $LIBS"

# Download prebuilt dependencies
# ------------------------------

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
./script/build/unix/install $PKG_OS-curl-$dep_curl_version                  \
                            $PKG_OS-geoip-api-c-$dep_geoip_api_c_version    \
                            $PKG_OS-libevent-$dep_libevent_version          \
                            $PKG_OS-libmaxminddb-$dep_libmaxminddb_version  \
                            $PKG_OS-libressl-$dep_libressl_version

dep_cppflags=(
  "-IMK_DIST/$PKG_OS/curl/$dep_curl_version/$PKG_ARCH/include"
  "-IMK_DIST/$PKG_OS/geoip-api-c/$dep_geoip_api_c_version/$PKG_ARCH/include"
  "-IMK_DIST/$PKG_OS/libevent/$dep_libevent_version/$PKG_ARCH/include"
  "-IMK_DIST/$PKG_OS/libmaxminddb/$dep_libmaxminddb_version/$PKG_ARCH/include"
  "-IMK_DIST/$PKG_OS/libressl/$dep_libressl_version/$PKG_ARCH/include"
)
for dep_cppflags_entry in ${dep_cppflags[*]}; do
  echo "* CPPFLAGS += $dep_cppflags_entry"
  CPPFLAGS="$CPPFLAGS $dep_cppflags_entry"
done

dep_libs=(
  "MK_DIST/$PKG_OS/geoip-api-c/$dep_geoip_api_c_version/$PKG_ARCH/lib/libGeoIP.a"
  "MK_DIST/$PKG_OS/libmaxminddb/$dep_libmaxminddb_version/$PKG_ARCH/lib/libmaxminddb.a"
  "MK_DIST/$PKG_OS/libevent/$dep_libevent_version/$PKG_ARCH/lib/libevent_core.a"
  "MK_DIST/$PKG_OS/libevent/$dep_libevent_version/$PKG_ARCH/lib/libevent_extra.a"
  "MK_DIST/$PKG_OS/libevent/$dep_libevent_version/$PKG_ARCH/lib/libevent_pthreads.a"
  "MK_DIST/$PKG_OS/libevent/$dep_libevent_version/$PKG_ARCH/lib/libevent_openssl.a"
  "MK_DIST/$PKG_OS/curl/$dep_curl_version/$PKG_ARCH/lib/libcurl.a"
  "MK_DIST/$PKG_OS/libressl/$dep_libressl_version/$PKG_ARCH/lib/libssl.a"
  "MK_DIST/$PKG_OS/libressl/$dep_libressl_version/$PKG_ARCH/lib/libcrypto.a"
)
for dep_libs_entry in ${dep_libs[*]}; do
  echo "* LIBS += $dep_libs_entry"
  LIBS="$LIBS $dep_libs_entry"
done

echo "CPPFLAGS += -DHAVE_BUFFEREVENT_OPENSSL_SET_ALLOW_DIRTY_SHUTDOWN"
CPPFLAGS="$CPPFLAGS -DHAVE_BUFFEREVENT_OPENSSL_SET_ALLOW_DIRTY_SHUTDOWN"

# Add extra compiler flags
# ------------------------

CFLAGS="$CFLAGS -Wall -Wextra -pedantic"
CXXFLAGS="$CXXFLAGS -Wall -Wextra -pedantic"
CPPFLAGS="$CPPFLAGS -DMK_NETTESTS_INTERNAL"

# Finalize compiler flags
# -----------------------

ninja_cxxflags="$CPPFLAGS -I. -Iinclude $CXXFLAGS -fPIC -std=c++14"
ninja_cflags="$CPPFLAGS -I. -Iinclude $CFLAGS -fPIC -std=c11"
ninja_depsflags="-MMD -MF \$out.d"
ninja_build_c="$CC $ninja_cflags $ninja_depsflags -c \$in -o \$out"
ninja_build_cpp="$CXX $ninja_cxxflags $ninja_depsflags -c \$in -o \$out"
ninja_link_exe="$CXX $LDFLAGS -o \$out \$in $LIBS"

echo "* ninja_build_c: $ninja_build_c"
echo "* ninja_build_cpp: $ninja_build_cpp"
echo "* ninja_link_exe: $ninja_link_exe"

# Write ninja.build
# -----------------

ofile=build.ninja
odir=.objects
rm -rf $ofile $odir
install -d $odir

echo "rule build_cpp"                                                >> $ofile
echo "  command = $ninja_build_cpp"                                  >> $ofile
echo "  depfile = \$out.d"                                           >> $ofile
echo "rule build_c"                                                  >> $ofile
echo "  command = $ninja_build_c"                                    >> $ofile
echo "  depfile = \$out.d"                                           >> $ofile
echo "rule ar"                                                       >> $ofile
echo "  command = $AR r \$out \$in"                                  >> $ofile
echo "rule link_exe"                                                 >> $ofile
echo "  command = $ninja_link_exe"                                   >> $ofile
echo ""                                                              >> $ofile

libname=libmeasurement_kit
echo "* Build $libname.a with:"
obj_list=""
for ext in c cpp; do
  for src_path in $(find src/$libname -type f -name \*.$ext); do
    obj_path="$odir/$(echo $src_path | sed -e "s/\\.$ext\$/.o/g" -e "s|/|_|g")"
    echo "  - $src_path"
    echo "build $obj_path: build_$ext $src_path"                     >> $ofile
    obj_list="$obj_list $obj_path"
  done
done
echo ""                                                              >> $ofile
echo "build $libname.a: ar $obj_list"                                >> $ofile
echo ""                                                              >> $ofile

binname=measurement_kit
obj_list=""
echo "* Build $binname with:"
for ext in c cpp; do
  for src_path in $(find src/$binname -type f -name \*.$ext); do
    obj_path="$odir/$(echo $src_path | sed -e "s/\\.$ext\$/.o/g" -e "s|/|_|g")"
    echo "  - $src_path"
    echo "build $obj_path: build_$ext $src_path"                     >> $ofile
    obj_list="$obj_list $obj_path"
  done
done
echo ""                                                              >> $ofile
echo "build $binname: link_exe $obj_list $libname.a"                 >> $ofile
echo ""                                                              >> $ofile

echo "build all: phony $libname.a $binname"                          >> $ofile
echo "default all"                                                   >> $ofile

echo "* Done. Build with \`ninja -v\`."
