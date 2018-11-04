AC_DEFUN([MK_AM_RESOLV], [
  mk_not_found=""
  AC_CHECK_HEADERS(resolv.h, [], [mk_not_found=1])
  AC_CHECK_LIB(resolv, res_nsend, [], [mk_not_found=1])
  AM_CONDITIONAL([HAVE_RESOLV], [test "$mk_not_found" != "1"])
])

AC_DEFUN([MK_AM_LIBEVENT], [

  AC_ARG_WITH([libevent],
              [AS_HELP_STRING([--with-libevent],
                [event I/O library @<:@default=check@:>@])
              ],
              [
                CPPFLAGS="$CPPFLAGS -I$withval/include"
                LDFLAGS="$LDFLAGS -L$withval/lib"
              ],
              [])

  mk_not_found=""
  AC_CHECK_HEADERS(event2/event.h, [], [mk_not_found=1])
  AC_CHECK_LIB(event, event_new, [], [mk_not_found=1])
  AC_CHECK_HEADERS(event2/thread.h, [], [mk_not_found=1])
  case $host in
    *-w64-mingw32)
      # nothing
    ;;
    *)
      AC_CHECK_LIB(event_pthreads, evthread_use_pthreads, [], [mk_not_found=1])
    ;;
  esac
  AC_CHECK_LIB(event_openssl, bufferevent_openssl_filter_new, [],
               [mk_not_found=1])

  AC_CHECK_FUNCS([bufferevent_openssl_set_allow_dirty_shutdown])

  if test "$mk_not_found" = "1"; then
    AC_MSG_WARN([Failed to find dependency: libevent])
    echo "    - to install on Debian: sudo apt-get install libevent-dev"
    echo "    - to install on OSX: brew install libevent"
    AC_MSG_ERROR([Please, install libevent and run configure again])
  fi
])

AC_DEFUN([MK_AM_GEOIP], [

  AC_ARG_WITH([geoip],
              [AS_HELP_STRING([--with-geoip],
                [GeoIP library @<:@default=check@:>@])
              ],
              [
                CPPFLAGS="$CPPFLAGS -I$withval/include"
                LDFLAGS="$LDFLAGS -L$withval/lib"
              ],
              [])

  mk_not_found=""
  AC_CHECK_HEADERS(GeoIP.h, [], [mk_not_found=1])
  AC_CHECK_LIB(GeoIP, GeoIP_open, [], [mk_not_found=1])

  if test "$mk_not_found" = "1"; then
    AC_MSG_WARN([Failed to find dependency: geoip])
    echo "    - to install on Debian: sudo apt-get install libgeoip-dev"
    echo "    - to install on OSX: brew install libgeoip"
    AC_MSG_ERROR([Please, install geoip and run configure again])
  fi
])

AC_DEFUN([MK_AM_LIBCURL], [
  AC_ARG_WITH([libcurl],
              [AS_HELP_STRING([--with-libcurl],
                [cURL library @<:@default=check@:>@])
              ],
              [
                CPPFLAGS="$CPPFLAGS -I$withval/include"
                LDFLAGS="$LDFLAGS -L$withval/lib"
              ],
              [])
  mk_not_found=""
  AC_CHECK_HEADERS(curl/curl.h, [], [mk_not_found=1])
  AC_CHECK_LIB(curl, curl_easy_init, [], [mk_not_found=1])
  if test "$mk_not_found" = "1"; then
    AC_MSG_WARN([Failed to find dependency: libcurl])
    echo "    - to install on Debian: sudo apt-get install libcurl4-openssl-dev"
    echo "    - to install on OSX: brew install curl"
    AC_MSG_ERROR([Please, install libcurl and run configure again])
  fi
])

AC_DEFUN([MK_AM_LIBMAXMINDDB], [
  AC_ARG_WITH([libmaxminddb],
              [AS_HELP_STRING([--with-libmaxminddb],
                [MaxMindDB library @<:@default=check@:>@])
              ],
              [
                CPPFLAGS="$CPPFLAGS -I$withval/include"
                LDFLAGS="$LDFLAGS -L$withval/lib"
              ],
              [])
  mk_not_found=""
  AC_CHECK_HEADERS(maxminddb.h, [], [mk_not_found=1])
  AC_CHECK_LIB(maxminddb, MMDB_open, [], [mk_not_found=1])
  if test "$mk_not_found" = "1"; then
    AC_MSG_WARN([Failed to find dependency: libmaxminddb])
    echo "    - to install on Debian: sudo apt-get install libmaxminddb-dev"
    echo "    - to install on OSX: brew install libmaxminddb"
    AC_MSG_ERROR([Please, install libmaxminddb and run configure again])
  fi
])

AC_DEFUN([MK_AM_OPENSSL], [

  AC_ARG_WITH([openssl],
              [AS_HELP_STRING([--with-openssl],
                [SSL toolkit @<:@default=check@:>@])
              ],
              [
                CPPFLAGS="$CPPFLAGS -I$withval/include"
                LDFLAGS="$LDFLAGS -L$withval/lib"
              ],
              [
	        if test -d /usr/local/Cellar/openssl; then
		  AC_MSG_WARN([Using the OpenSSL installed via brew...])
		  mk_openssl_v=`ls /usr/local/Cellar/openssl|tail -n1`
		  mk_openssl_d="/usr/local/Cellar/openssl/$mk_openssl_v"
		  CPPFLAGS="$CPPFLAGS -I$mk_openssl_d/include"
		  LDFLAGS="$LDFLAGS -L$mk_openssl_d/lib"
		fi
	      ])

  mk_not_found=""
  AC_CHECK_HEADERS(openssl/ssl.h, [], [mk_not_found=1])
  AC_CHECK_LIB(crypto, RSA_new, [], [mk_not_found=1])
  AC_CHECK_LIB(ssl, SSL_new, [], [mk_not_found=1])

  dnl This test breaks the build with 12.04 on travis because the linker there
  dnl requires `LD_RUN_PATH` which sadly is not honoured by this test, still
  dnl no worries because actually this check only makes sense for Mac systems
  if test "`uname`" = "Darwin"; then
    AC_MSG_CHECKING([whether OpenSSL is older than 1.0.0])
    AC_RUN_IFELSE([
      AC_LANG_SOURCE([
      #include <openssl/opensslv.h>
      #include <openssl/ssl.h>

      /* Code taken from tor/src/common/crypto.h */

      #define OPENSSL_VER(a,b,c,d,e)                                \
        (((a)<<28) |                                                \
         ((b)<<20) |                                                \
         ((c)<<12) |                                                \
         ((d)<< 4) |                                                \
          (e))
      #define OPENSSL_V_SERIES(a,b,c) OPENSSL_VER((a),(b),(c),0,0)

      /* Code taken from tor/src/common/compat_openssl.h */

      #if OPENSSL_VERSION_NUMBER < OPENSSL_V_SERIES(1,0,0)
      # error "We require OpenSSL >= 1.0.0"
      #endif

      #if OPENSSL_VERSION_NUMBER >= OPENSSL_V_SERIES(1,1,0) && \
          ! defined(LIBRESSL_VERSION_NUMBER)
      /* We define this macro if we're trying to build with the majorly refactored
       * API in OpenSSL 1.1 */
      #define OPENSSL_1_1_API
      #endif

      #ifndef OPENSSL_1_1_API
      #define OpenSSL_version_num() SSLeay()
      #endif

      int main() {
        if (OpenSSL_version_num() != OPENSSL_VERSION_NUMBER) {
          return 1;
        }
        if (OpenSSL_version_num() < OPENSSL_V_SERIES(1, 0, 0)) {
          return 2;
        }
        return 0;
      }
      ])
    ],
    [AC_MSG_RESULT([yes])],
    [
      AC_MSG_RESULT([no])
      if test -d /usr/local/Cellar/openssl; then
        AC_MSG_WARN([MacOS ships an old system-wide OpenSSL but you seem to])
        AC_MSG_WARN([have a new version installed with brew.])
        AC_MSG_WARN([So, you should try adding this flag to configure:])
        AC_MSG_WARN(['--with-openssl=/usr/local/Cellar/openssl/VERSION/'])
      fi
      mk_not_found=1
    ],
    [AC_MSG_RESULT([Skip the test because we are cross-compiling])])
  fi

  if test "$mk_not_found" = "1"; then
    AC_MSG_WARN([Failed to find dependency: openssl])
    echo "    - to install on Debian: sudo apt-get install libssl-dev"
    echo "    - to install on OSX: brew install openssl"
    AC_MSG_ERROR([Please, install openssl and run configure again])
  fi
])

AC_DEFUN([MK_MAYBE_ADD_CFLAG], [
  AX_SAVE_FLAGS([mk])
  mk_maybe_add_flag=false
  CFLAGS="$1 -Werror"
  AC_MSG_CHECKING([whether $CC supports $1])
  AC_LANG_PUSH([C])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
                    [AC_MSG_RESULT([yes])
                     mk_maybe_add_flag=true],
                    [AC_MSG_RESULT([no])])
  AC_LANG_POP([C])
  AX_RESTORE_FLAGS([mk])
  if [ "$mk_maybe_add_flag" = "true" ]; then CFLAGS="$CFLAGS $1"; fi
])

AC_DEFUN([MK_MAYBE_ADD_CXXFLAG], [
  AX_SAVE_FLAGS([mk])
  mk_maybe_add_flag=false
  CXXFLAGS="$1 -Werror"
  AC_MSG_CHECKING([whether $CXX supports $1])
  AC_LANG_PUSH([C++])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
                    [AC_MSG_RESULT([yes])
                     mk_maybe_add_flag=true],
                    [AC_MSG_RESULT([no])])
  AC_LANG_POP([C++])
  AX_RESTORE_FLAGS([mk])
  if [ "$mk_maybe_add_flag" = "true" ]; then CXXFLAGS="$CXXFLAGS $1"; fi
])

AC_DEFUN([MK_MAYBE_ADD_LDFLAG], [
  AX_SAVE_FLAGS([mk])
  mk_maybe_add_flag=false
  LDFLAGS="$1 -Werror"
  AC_MSG_CHECKING([whether $CC supports $1])
  AC_LANG_PUSH([C])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([])],
                 [AC_MSG_RESULT([yes])
                  mk_maybe_add_flag=true],
                 [AC_MSG_RESULT([no])])
  AC_LANG_POP([C])
  AX_RESTORE_FLAGS([mk])
  if [ "$mk_maybe_add_flag" = "true" ]; then LDFLAGS="$LDFLAGS $1"; fi
])

AC_DEFUN([MK_REQUIRE_CFLAG], [
  AX_SAVE_FLAGS([mk])
  CFLAGS="$1 -Werror"
  AC_MSG_CHECKING([whether $CC supports $1])
  AC_LANG_PUSH([C])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], [AC_MSG_RESULT([yes])],
                    [AC_MSG_RESULT([no])
                     AC_MSG_ERROR([$CC does not support $1])])
  AC_LANG_POP([C])
  AX_RESTORE_FLAGS([mk])
  CFLAGS="$CFLAGS $1"
])

AC_DEFUN([MK_REQUIRE_CXXFLAG], [
  AX_SAVE_FLAGS([mk])
  CXXFLAGS="$1 -Werror"
  AC_MSG_CHECKING([whether $CXX supports $1])
  AC_LANG_PUSH([C++])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])], [AC_MSG_RESULT([yes])],
                    [AC_MSG_RESULT([no])
                     AC_MSG_ERROR([$CXX does not support $1])])
  AC_LANG_POP([C++])
  AX_RESTORE_FLAGS([mk])
  CXXFLAGS="$CXXFLAGS $1"
])

AC_DEFUN([MK_MAYBE_CA_BUNDLE], [
  AC_MSG_CHECKING([CA bundle path])
  AC_ARG_WITH([ca-bundle], AC_HELP_STRING([--with-ca-bundle=FILE],
               [Path to CA bundle]), [want_ca="$withval"], [want_ca="unset"])
  if test "x$want_ca" != "xunset"; then
    ca="$want_ca"
  else
    ca="no"
    if test "x$cross_compiling" != "xyes"; then
        for a in /etc/ssl/certs/ca-certificates.crt                            \
                 /etc/pki/tls/certs/ca-bundle.crt                              \
                 /usr/share/ssl/certs/ca-bundle.crt                            \
                 /usr/local/share/certs/ca-root.crt                            \
                 /etc/ssl/cert.pem                                             \
                 /usr/local/etc/openssl/cert.pem; do
          if test -f "$a"; then
            ca="$a"
            break
          fi
        done
    fi
  fi
  if test "x$ca" != "xno"; then
    MK_CA_BUNDLE="$ca"
    AC_DEFINE_UNQUOTED(MK_CA_BUNDLE, "$ca", [Location of default ca bundle])
    AC_SUBST(MK_CA_BUNDLE)
    AC_MSG_RESULT([$ca])
  elif test "x$cross_compiling" == "xyes"; then
    AC_MSG_RESULT([skipped (cross compiling)])
    AC_MSG_WARN([skipped the ca-bundle detection when cross-compiling])
    AC_DEFINE_UNQUOTED(MK_CA_BUNDLE, "", [Location of default ca bundle])
    AC_SUBST(MK_CA_BUNDLE)
  else
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([you should give a ca-bundle location])
  fi
])