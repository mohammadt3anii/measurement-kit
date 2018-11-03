// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#ifdef ENABLE_INTEGRATION_TESTS
#include "src/libmeasurement_kit/ext/catch.hpp"

#include "../nettests/utils.hpp"

using namespace mk::nettests;
using namespace mk;

TEST_CASE("Captive portal test") {
    test::nettests::with_test<CaptivePortalTest>(test::nettests::run_test);
}

#endif
