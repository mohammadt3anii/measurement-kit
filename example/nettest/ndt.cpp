// Public domain 2017, Simone Basso <bassosimone@gmail.com.

#include "test/winsock.hpp"

#define MK_NETTEST_VERBOSE_DEFAULT_HANDLERS  // print events in handlers
#include <measurement_kit/experimental/nettest.hpp>

#include <stdlib.h>

#include <iostream>

int main() {
    mk::nettest::NdtSettings settings;
    settings.log_level = mk::nettest::log_level_info;
    mk::nettest::Nettest nettest{std::move(settings)};
    if (!nettest.run()) {
        std::clog << "ERROR: running nettest failed" << std::endl;
        exit(1);
    }
}
