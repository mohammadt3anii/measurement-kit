// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#define CATCH_CONFIG_MAIN
#include "src/libmeasurement_kit/ext/catch.hpp"

#include "utils.hpp"

using namespace mk::nettests;

#ifdef ENABLE_INTEGRATION_TESTS

TEST_CASE("Synchronous dns-injection test") {
    test::nettests::with_test<DnsInjectionTest>(
          "hosts.txt",
          [](BaseTest &test) { test.set_option("dns/timeout", "0.1").run(); });
}

#endif
