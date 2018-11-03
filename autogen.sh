#!/bin/sh

set -e
export LC_ALL=C  # Stable sorting regardless of the locale

. script/autogen.d/geoip

no_geoip=0
cmake=0
while [ $# -gt 0 ]; do
    opt=$1; shift
    if [ "$opt" = "--no-geoip" -o "$opt" = "-n" ]; then
        no_geoip=1
    elif [ "$opt" = "--cmake" ]; then
        cmake=1
    else
        echo "usage: $0 [--cmake] [-n|--no-geoip]" 1>&2
        exit 1
    fi
done

if [ $cmake -ne 0 ]; then
    ./script/cmake/autogen
fi

if [ $no_geoip -ne 1 ]; then
    autogen_get_geoip
fi

if [ $cmake -eq 0 ]; then
    echo "* Running 'autoreconf -i'"
    autoreconf -i
fi

echo ""
echo "=== autogen.sh complete ==="
echo ""

if [ $cmake -eq 0 ]; then
    echo "MeasurementKit is now ready to be compiled. To proceed you shall run"
    echo "now the './configure' script that adapts the build to your system."
    echo ""
    echo "The './configure' script shall also check for external dependencies. "
    echo "MeasurementKit external dependencies are:"
    echo ""
    echo "    - GeoIP"
    echo "    - openssl / libressl"
    echo "    - libevent"
    echo "    - libcurl"
    echo "    - libmaxminddb"
    echo "    - zlib"
    echo ""
    echo "If any of these dependencies is missing, the './configure' script"
    echo "shall stop and tell you how you could install it."
else
    echo "MeasurementKit is now ready to be compiled. To proceed you shall run"
    echo "either 'cmake' directly or our friendly './script/cmake/run' wrapper"
    echo "script. The latter attempts to apply smart settings in case you are"
    echo "running 'cmake' on macOS and have brew installed."
fi

echo ""
