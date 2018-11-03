// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/common/freverse.hpp"

#include <measurement_kit/common.hpp>

TEST_CASE("mk::freverse() works as expected") {
    SECTION("For empty tuple") {
        REQUIRE(mk::freverse(std::make_tuple()) == std::make_tuple());
    }

    SECTION("For one-element tuples") {
        REQUIRE(mk::freverse(std::make_tuple(1)) == std::make_tuple(1));
    }

    SECTION("For two-element tuples") {
        REQUIRE(mk::freverse(std::make_tuple(1, 2)) == std::make_tuple(2, 1));
    }

    SECTION("For three-element tuples") {
        REQUIRE(mk::freverse(std::make_tuple(1, 2, 3)) ==
                std::make_tuple(3, 2, 1));
    }
}
