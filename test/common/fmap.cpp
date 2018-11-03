// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/common/continuation.hpp"
#include "src/libmeasurement_kit/common/fmap.hpp"

#include <measurement_kit/common.hpp>

using namespace mk;

TEST_CASE("mk::fmap() works as expected for empty vector") {
    std::vector<int> output = mk::fmap<int, int>({}, [](int i) {
        return i * 2;
    });
    REQUIRE((output.size() <= 0));
}

TEST_CASE("mk::fmap() works as expected for simple numbers") {
    std::vector<int> input{1, 3, 5};
    std::vector<double> output = mk::fmap<int, double>(input, [](int i) {
        return i / 10.0;
    });
    REQUIRE((output.size() > 0));
    REQUIRE((output == std::vector<double>{0.1, 0.3, 0.5}));
}

TEST_CASE("mk::fmap() allows to construct continuations") {
    std::vector<int> input{1, 4};
    std::vector<Continuation<int>> output = mk::fmap<int, Continuation<int>>(
            input,
            [](int i) {
                return [=](Callback<int> callback) {
                    callback(i);
                };
            });
    REQUIRE((output.size() > 0));
    output[0]([](int i) { REQUIRE((i == 1)); });
    output[1]([](int i) { REQUIRE((i == 4)); });
}
