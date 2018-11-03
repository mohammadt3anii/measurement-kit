// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "test/winsock.hpp"

#include "src/libmeasurement_kit/ext/catch.hpp"

#include "src/libmeasurement_kit/common/utils.hpp"
#include "src/libmeasurement_kit/report/base_reporter.hpp"
#include "src/libmeasurement_kit/report/error.hpp"
#include "src/libmeasurement_kit/report/report.hpp"

using namespace mk;
using namespace mk::report;

class CountedReporter : public BaseReporter {
  public:
    static SharedPtr<CountedReporter> make() {
        return SharedPtr<CountedReporter>(new CountedReporter);
    }

    ~CountedReporter() override;

    Continuation<Error> open(Report &) override {
        return do_open_([=](Callback<Error> cb) {
            ++open_count;
            return cb(NoError());
        });
    }

    Continuation<Error> write_entry(Entry e) override {
        return do_write_entry_(e, [=](Callback<Error> cb) {
            ++write_count;
            return cb(NoError());
        });
    }

    Continuation<Error> close() override {
        return do_close_([=](Callback<Error> cb) {
            ++close_count;
            return cb(NoError());
        });
    }

    int close_count = 0;
    int open_count = 0;
    int write_count = 0;
};

CountedReporter::~CountedReporter() {}

class FailingReporter : public BaseReporter {
  public:
    static SharedPtr<FailingReporter> make() {
        return SharedPtr<FailingReporter>(new FailingReporter);
    }

    ~FailingReporter() override;

    Continuation<Error> open(Report & /*report*/) override {
        return do_open_([=](Callback<Error> cb) {
            if (open_count++ == 0) {
                cb(MockedError());
                return;
            }
            cb(NoError());
        });
    }

    Continuation<Error> write_entry(Entry e) override {
        return do_write_entry_(e, [=](Callback<Error> cb) {
            if (write_count++ == 0) {
                cb(MockedError());
                return;
            }
            cb(NoError());
        });
    }

    Continuation<Error> close() override {
        return do_close_([=](Callback<Error> cb) {
            if (close_count++ == 0) {
                cb(MockedError());
                return;
            }
            cb(NoError());
        });
    }

    int close_count = 0;
    int open_count = 0;
    int write_count = 0;
};

FailingReporter::~FailingReporter() {}

TEST_CASE("Report constructor works correctly") {
    REQUIRE_NOTHROW(Report());
}

TEST_CASE("The open() method works correctly") {
    Report report;
    report.add_reporter(BaseReporter::make());
    report.open([&](Error err) {
        REQUIRE(!err);
        report.open([&](Error err) {
            REQUIRE(err == NoError());
            REQUIRE(err.child_errors.size() == 1);
            REQUIRE(err.child_errors[0] == NoError());
            REQUIRE(err.child_errors[0].child_errors.size() == 1);
            REQUIRE(err.child_errors[0].child_errors[0] ==
                    ReportAlreadyOpenError());
        });
    });
}

TEST_CASE("We can retry a partially successful open") {
    SharedPtr<CountedReporter> counted_reporter = CountedReporter::make();
    SharedPtr<FailingReporter> failing_reporter = FailingReporter::make();
    Report report;
    report.add_reporter(counted_reporter.as<BaseReporter>());
    report.add_reporter(failing_reporter.as<BaseReporter>());
    report.open([&](Error err) {
        REQUIRE(err == ParallelOperationError());
        REQUIRE(err.child_errors.size() == 2);
        REQUIRE(err.child_errors[0] == NoError());
        REQUIRE(err.child_errors[1] == MockedError());
        report.open([&](Error err) {
            REQUIRE(err == NoError());
            REQUIRE(err.child_errors.size() == 2);
            REQUIRE(err.child_errors[0] == NoError());
            REQUIRE(err.child_errors[0].child_errors.size() == 1);
            REQUIRE(err.child_errors[0].child_errors[0] ==
                    ReportAlreadyOpenError());
            REQUIRE(err.child_errors[1] == NoError());
        });
    });
    REQUIRE(counted_reporter->open_count == 1);
    REQUIRE(failing_reporter->open_count == 2);
}

TEST_CASE("The write_entry() method works correctly") {
    Report report;
    report.add_reporter(BaseReporter::make());
    Entry entry;
    report.write_entry(entry, [&](Error err) {
        REQUIRE(err == ParallelOperationError());
        REQUIRE(err.child_errors.size() == 1);
        REQUIRE(err.child_errors[0] == ReportNotOpenError());
        report.open([&](Error err) {
            REQUIRE(!err);
            entry["foobar"] = 1;
            report.write_entry(entry, [&](Error err) {
                REQUIRE(!err);
                REQUIRE(err.child_errors.size() == 1);
                REQUIRE(err.child_errors[0] == NoError());
                REQUIRE(err.child_errors[0].child_errors.size() == 0);
                entry["foobar"] = 2;
                report.write_entry(entry, [&](Error err) {
                    REQUIRE(!err);
                    REQUIRE(err.child_errors.size() == 1);
                    REQUIRE(err.child_errors[0] == NoError());
                    REQUIRE(err.child_errors[0].child_errors.size() == 0);
                    entry["foobar"] = 3;
                    report.write_entry(entry, [&](Error err) {
                        REQUIRE(!err);
                        report.close([&](Error err) {
                            REQUIRE(!err);
                            REQUIRE(err.child_errors.size() == 1);
                            REQUIRE(
                                err.child_errors[0] == NoError());
                            REQUIRE(
                                err.child_errors[0].child_errors.size() == 0);
                            entry["foobar"] = 4;
                            report.write_entry(entry, [&](Error err) {
                                REQUIRE(err ==
                                        ParallelOperationError());
                                REQUIRE(err.child_errors.size() == 1);
                                REQUIRE(err.child_errors[0] ==
                                        ReportAlreadyClosedError());
                            }, Logger::global());
                        });
                    }, Logger::global());
                }, Logger::global());
            }, Logger::global());
        });
    }, Logger::global());
}

TEST_CASE("We can retry a partially successful write_entry()") {
    SharedPtr<CountedReporter> counted_reporter = CountedReporter::make();
    SharedPtr<FailingReporter> failing_reporter = FailingReporter::make();
    failing_reporter->open_count = 1; // So open won't fail
    Report report;
    report.add_reporter(counted_reporter.as<BaseReporter>());
    report.add_reporter(failing_reporter.as<BaseReporter>());
    report.open([&](Error err) {
        REQUIRE(err == NoError());
        Entry entry;
        entry["foobar"] = 17;
        entry["baz"] = "foobar";
        report.write_entry(entry, [&](Error err) {
            REQUIRE(err == ParallelOperationError());
            REQUIRE(err.child_errors.size() == 2);
            REQUIRE(err.child_errors[0] == NoError());
            REQUIRE(err.child_errors[1] == MockedError());
            report.write_entry(entry, [&](Error err) {
                REQUIRE(err == NoError());
                REQUIRE(err.child_errors.size() == 2);
                REQUIRE(err.child_errors[0] == NoError());
                REQUIRE(err.child_errors[0].child_errors.size() == 1);
                REQUIRE(err.child_errors[0].child_errors[0] ==
                        DuplicateEntrySubmitError());
                REQUIRE(err.child_errors[1] == NoError());
            }, Logger::global());
        }, Logger::global());
    });
    REQUIRE(counted_reporter->write_count == 1);
    REQUIRE(failing_reporter->write_count == 2);
}

TEST_CASE("The close() method works correctly") {
    Report report;
    report.add_reporter(BaseReporter::make());
    report.open([&](Error err) {
        REQUIRE(!err);
        report.close([&](Error err) {
            REQUIRE(!err);
            report.close([&](Error err) {
                REQUIRE(err == NoError());
                REQUIRE(err.child_errors.size() == 1);
                REQUIRE(err.child_errors[0] == NoError());
                REQUIRE(err.child_errors[0].child_errors.size() == 1);
                REQUIRE(err.child_errors[0].child_errors[0] ==
                        ReportAlreadyClosedError());
            });
        });
    });
}

TEST_CASE("We can retry a partially successful close") {
    SharedPtr<CountedReporter> counted_reporter = CountedReporter::make();
    SharedPtr<FailingReporter> failing_reporter = FailingReporter::make();
    failing_reporter->open_count = 1; // So open won't fail
    Report report;
    report.add_reporter(counted_reporter.as<BaseReporter>());
    report.add_reporter(failing_reporter.as<BaseReporter>());
    report.open([&](Error err) {
        REQUIRE(err == NoError());
        report.close([&](Error err) {
            REQUIRE(err == ParallelOperationError());
            REQUIRE(err.child_errors.size() == 2);
            REQUIRE(err.child_errors[0] == NoError());
            REQUIRE(err.child_errors[1] == MockedError());
            report.close([&](Error err) {
                REQUIRE(err == NoError());
                REQUIRE(err.child_errors.size() == 2);
                REQUIRE(err.child_errors[0] == NoError());
                REQUIRE(err.child_errors[0].child_errors.size() == 1);
                REQUIRE(err.child_errors[0].child_errors[0] ==
                        ReportAlreadyClosedError());
                REQUIRE(err.child_errors[1] == NoError());
            });
        });
    });
    REQUIRE(counted_reporter->close_count == 1);
    REQUIRE(failing_reporter->close_count == 2);
}

TEST_CASE("We can override software name and version") {
    Report report;
    Entry entry;

    // We must set the `test_start_time` because otherwise the
    // test fails with an `error_or_contains_error` failure.
    mk::utc_time_now(&report.test_start_time);

    SECTION("We have a default") {
        report.fill_entry(entry);
        REQUIRE(entry["software_name"] == report.software_name);
        REQUIRE(entry["software_version"] == report.software_version);
    }

    SECTION("We can override the default") {
        report.options["software_name"] = "ooniprobe-ios";
        report.options["software_version"] = "1.0.1";
        report.fill_entry(entry);
        REQUIRE(entry["software_name"] == "ooniprobe-ios");
        REQUIRE(entry["software_version"] == "1.0.1");
    }
}
