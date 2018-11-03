// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/ndt/test_meta_impl.hpp"
#include "src/libmeasurement_kit/net/emitter.hpp"

using namespace mk;
using namespace mk::ndt;
using namespace mk::net;

static void fail(SharedPtr<Context>, Callback<Error, uint8_t, std::string> cb,
                 SharedPtr<Reactor> = Reactor::global()) {
    cb(MockedError(), 0, "");
}

TEST_CASE("run() deals with read() error") {
    SharedPtr<Context> ctx(new Context);
    test_meta::run_impl<fail>(
        ctx, [](Error err) { REQUIRE(err == ReadingTestPrepareError()); });
}

static void unexpected(SharedPtr<Context>, Callback<Error, uint8_t, std::string> cb,
                       SharedPtr<Reactor> = Reactor::global()) {
    cb(NoError(), MSG_ERROR, "");
}

TEST_CASE("run() deals with unexpected message type") {
    SharedPtr<Context> ctx(new Context);
    test_meta::run_impl<unexpected>(
        ctx, [](Error err) { REQUIRE(err == NotTestPrepareError()); });
}

static void test_prepare(SharedPtr<Context>,
                         Callback<Error, uint8_t, std::string> cb,
                         SharedPtr<Reactor> = Reactor::global()) {
    cb(NoError(), TEST_PREPARE, "");
}

TEST_CASE("run() deals with second read() error") {
    SharedPtr<Context> ctx(new Context);
    test_meta::run_impl<test_prepare, fail>(
        ctx, [](Error err) { REQUIRE(err == ReadingTestStartError()); });
}

TEST_CASE("run() deals with unexpected message on second read") {
    SharedPtr<Context> ctx(new Context);
    test_meta::run_impl<test_prepare, unexpected>(
        ctx, [](Error err) { REQUIRE(err == NotTestStartError()); });
}

static ErrorOr<Buffer> fail(std::string) { return {MockedError(), {}}; }

static ErrorOr<Buffer> success(std::string s) {
    Buffer buff;
    buff.write(s);
    return {NoError(), buff};
}

static void test_start(SharedPtr<Context>, Callback<Error, uint8_t, std::string> cb,
                       SharedPtr<Reactor> = Reactor::global()) {
    cb(NoError(), TEST_START, "");
}

TEST_CASE("run() deals with first format_test_msg() error") {
    SharedPtr<Context> ctx(new Context);
    test_meta::run_impl<test_prepare, test_start, fail>(ctx, [](Error err) {
        REQUIRE(err == SerializingClientVersionError());
    });
}

TEST_CASE("run() deals with second format_test_msg() error") {
    SharedPtr<Context> ctx(new Context);
    ctx->txp.reset(new Emitter(Reactor::global(), Logger::global()));
    test_meta::run_impl<test_prepare, test_start, success, fail>(
        ctx,
        [](Error err) { REQUIRE(err == SerializingClientApplicationError()); });
}

TEST_CASE("run() deals with third format_test_msg() error") {
    SharedPtr<Context> ctx(new Context);
    ctx->txp.reset(new Emitter(Reactor::global(), Logger::global()));
    test_meta::run_impl<test_prepare, test_start, success, success, fail>(
        ctx, [](Error err) { REQUIRE(err == SerializingFinalMetaError()); });
}

static void fail(SharedPtr<Context>, Buffer, Callback<Error> cb) {
    cb(MockedError());
}

TEST_CASE("run() deals with write error") {
    SharedPtr<Context> ctx(new Context);
    ctx->txp.reset(new Emitter(Reactor::global(), Logger::global()));
    test_meta::run_impl<test_prepare, test_start, success, success, success,
                        fail>(
        ctx, [](Error err) { REQUIRE(err == WritingMetaError()); });
}

static void success(SharedPtr<Context>, Buffer, Callback<Error> cb) { cb(NoError()); }

TEST_CASE("run() deals with third read() error") {
    SharedPtr<Context> ctx(new Context);
    ctx->txp.reset(new Emitter(Reactor::global(), Logger::global()));
    test_meta::run_impl<test_prepare, test_start, success, success, success,
                        success, fail>(
        ctx, [](Error err) { REQUIRE(err == ReadingTestFinalizeError()); });
}

TEST_CASE("run() deals with unexpected message on third read") {
    SharedPtr<Context> ctx(new Context);
    ctx->txp.reset(new Emitter(Reactor::global(), Logger::global()));
    test_meta::run_impl<test_prepare, test_start, success, success, success,
                        success, unexpected>(
        ctx, [](Error err) { REQUIRE(err == NotTestFinalizeError()); });
}
