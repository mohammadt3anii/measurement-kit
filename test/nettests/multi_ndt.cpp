// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#ifdef ENABLE_INTEGRATION_TESTS

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "utils.hpp"

using namespace mk::nettests;

TEST_CASE("Synchronous multi NDT test") {
    test::nettests::with_test<MultiNdtTest>(test::nettests::run_test);
}

#endif
