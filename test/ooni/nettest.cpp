#define CATCH_CONFIG_MAIN
#include "src/ext/Catch/single_include/catch.hpp"

#include "ooni/nettest.hpp"
#include "common/poller.h"
#include "common/log.h"
#include "common/utils.h"

using namespace ight::ooni::nettest;

TEST_CASE("The NetTest should callback when it has finished running") {
  NetTest nettest("");
  nettest.begin([&](){
    nettest.end([](){
      ight_break_loop();
    });
  });
  ight_loop();
}
