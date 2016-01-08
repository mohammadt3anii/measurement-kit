// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef MEASUREMENT_KIT_OONI_NET_TEST_HPP
#define MEASUREMENT_KIT_OONI_NET_TEST_HPP

#include <iterator>
#include <iostream>
#include <fstream>

#include "src/report/file_reporter.hpp"

#include "src/common/delayed_call.hpp"
#include <measurement_kit/common/poller.hpp>
#include <measurement_kit/common/settings.hpp>
#include <measurement_kit/common/logger.hpp>
#include <measurement_kit/common/net_test.hpp>

namespace mk {
namespace ooni {

class InputGenerator {

  public:
    virtual void next(std::function<void(std::string)> &&new_line,
                      std::function<void()> &&done) = 0;

    virtual ~InputGenerator() {}
};

class InputFileGenerator : public InputGenerator {
  public:
    InputFileGenerator() {}

    InputFileGenerator(std::string input_filepath,
                       Logger *lp = Logger::global())
        : logger(lp) {
        is = new std::ifstream(input_filepath);
    }

    virtual ~InputFileGenerator() { delete is; /* delete handles nullptr */ }

    InputFileGenerator(InputFileGenerator &) = delete;
    InputFileGenerator &operator=(InputFileGenerator &) = delete;
    InputFileGenerator(InputFileGenerator &&) = default;
    InputFileGenerator &operator=(InputFileGenerator &&) = default;

    void next(std::function<void(std::string)> &&new_line,
              std::function<void()> &&done) override {
        logger->debug("InputFileGenerator: getting next line");
        std::string line;
        if (*is && !std::getline(*is, line).eof()) {
            logger->debug("InputFileGenerator: returning new line");
            new_line(line);
        } else {
            logger->debug("InputFileGenerator: EOF reached.");
            done();
        }
    }

  private:
    std::ifstream *is = nullptr;
    Logger *logger = Logger::global();
};

class OoniTest : public mk::NetTest {
    std::string input_filepath;
    report::FileReporter file_report;
    std::string report_filename;

    DelayedCall delayed_call;

    void run_next_measurement(const std::function<void()> &&cb);

    void geoip_lookup();

    void write_header();

    std::string generate_report_filename();

  protected:
    Libs *libs = Libs::global();

    virtual void setup(std::string input);
    virtual void setup();

    virtual void teardown(std::string input);
    virtual void teardown();

    virtual void main(Settings options,
                      std::function<void(report::Entry)> &&func);

    virtual void main(std::string input, Settings options,
                      std::function<void(report::Entry)> &&func);

  public:
    report::Entry entry;
    Settings options;
    InputGenerator *input = nullptr;

    std::string test_name;
    std::string test_version;

    std::string probe_ip = "127.0.0.1";
    std::string probe_asn = "AS0";
    std::string probe_cc = "ZZ";

    OoniTest(void);

    virtual ~OoniTest(void);

    OoniTest(OoniTest &) = delete;
    OoniTest &operator=(OoniTest &) = delete;
    OoniTest(OoniTest &&) = default;
    OoniTest &operator=(OoniTest &&) = default;

    OoniTest(std::string input_filepath);

    OoniTest(std::string input_filepath, Settings options);

    void set_report_filename(std::string s) {
        report_filename = s;
    }

    std::string get_report_filename() {
        if (report_filename == "") {
            report_filename = generate_report_filename();
        }
        return report_filename;
    }

    InputGenerator *input_generator();

    /*!
     * \brief Start iterating over the input.
     * \param cb Callback called when we are done.
     */
    virtual void begin(std::function<void()> cb) override;

    /*!
     * \brief Make sure that the report is written.
     * \param cb Callback called when the report is written.
     */
    virtual void end(std::function<void()> cb) override;
};

} // namespace ooni
} // namespace mk
#endif
