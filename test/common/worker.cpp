// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/common/worker.hpp"
#include "src/libmeasurement_kit/common/range.hpp"

#include <measurement_kit/common.hpp>

#include <chrono>
#include <iostream>
#include <thread>

TEST_CASE("The worker is robust to submitting many tasks in a row") {
    auto worker = mk::SharedPtr<mk::Worker>::make();
    for (auto _: mk::range<int>(128)) {
        worker->call_in_thread(mk::Logger::global(), []() {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(2s);
        });
    }
    for (;;) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        auto concurrency = worker->concurrency();
        std::cout << "Concurrency: " << concurrency << "\n";
        REQUIRE(concurrency <= worker->parallelism());
        if (concurrency == 0) {
            break;
        }
    }
}
