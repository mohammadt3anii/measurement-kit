// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "src/libmeasurement_kit/common/fmap.hpp"
#include "src/libmeasurement_kit/common/parallel.hpp"
#include "src/libmeasurement_kit/common/range.hpp"
#include "src/libmeasurement_kit/nettests/runnable.hpp"

#include "src/libmeasurement_kit/common/utils.hpp"
#include "src/libmeasurement_kit/ooni/bouncer.hpp"
#include "src/libmeasurement_kit/ooni/utils.hpp"
#include "src/libmeasurement_kit/nettests/utils.hpp"
#include "src/libmeasurement_kit/ext/sole.hpp"

#include "src/libmeasurement_kit/report/file_reporter.hpp"
#include "src/libmeasurement_kit/report/ooni_reporter.hpp"

#include <measurement_kit/nettests.hpp>

namespace mk {
namespace nettests {

using namespace mk::report;
using namespace mk::ooni;

Runnable::~Runnable() {}

void Runnable::setup(std::string) {}
void Runnable::teardown(std::string) {}
void Runnable::main(std::string, Settings, Callback<SharedPtr<report::Entry>> cb) {
    // Deferring calling `cb` a little bit to better emulate real tests
    // workflow because no test actually completes "immediately".
    reactor->call_later(0.74,
        [=]() { cb(SharedPtr<report::Entry>{new report::Entry}); });
}
void Runnable::fixup_entry(report::Entry &) {}

void Runnable::run_next_measurement(size_t thread_id, Callback<Error> cb,
                                    size_t num_entries,
                                    SharedPtr<size_t> current_entry) {
    logger->debug("net_test: running next measurement");

    double max_rt = options.get("max_runtime", -1.0);
    double max_rt_tolerance = max_rt / 10.0;
    double delta = mk::time_now() - beginning;
    if (max_rt >= 0.0 && delta > max_rt - max_rt_tolerance) {
        logger->info("Exceeded test maximum runtime");
        cb(NoError());
        return;
    }

    if (inputs.size() <= 0) {
        logger->debug("net_test: reached end of input");
        cb(NoError());
        return;
    }
    std::string next_input = inputs.front();
    inputs.pop_front();

    double prog = 0.0;
    if (max_rt > 0.0) {
        prog = delta / max_rt;
    } else if (num_entries > 0) {
        prog = *current_entry / (double)num_entries;
    }
    auto saved_current_entry = *current_entry; // used for emitting events
    *current_entry += 1;
    if (next_input != "") {
        std::string description;
        description += "Processing input: ";
        description += next_input;
        logger->progress(prog, description.c_str());
    }

    logger->debug("net_test: creating entry");
    struct tm measurement_start_time;
    double start_time;
    mk::utc_time_now(&measurement_start_time);
    start_time = mk::time_now();

    logger->debug("net_test: calling setup");
    setup(next_input);

    logger->debug("net_test: running with input %s", next_input.c_str());
    logger->emit_event_ex("status.measurement_start", nlohmann::json::object({
        {"idx", saved_current_entry},
        {"input", next_input},
    }));

    main(next_input, options, [=](SharedPtr<report::Entry> test_keys) {
        report::Entry entry;
        entry["input"] = next_input;
        // Make sure the input is `null` rather than empty string
        if (entry["input"] == "") {
            entry["input"] = nullptr;
        }
        entry["test_keys"] = *test_keys;
        entry["test_keys"]["client_resolver"] = resolver_ip;
        entry["measurement_start_time"] =
            *mk::timestamp(&measurement_start_time);
        entry["test_runtime"] = mk::time_now() - start_time;
        entry["id"] = mk::sole::uuid4().str();

        // Until we have support for passing options, leave it empty
        entry["options"] = Entry::array();

        // Until we have support for it, just put `null`
        entry["probe_city"] = nullptr;

        // Add test helpers
        entry["test_helpers"] = Entry::object();
        for (auto &name : test_helpers_option_names()) {
            if (options.count(name) != 0) {
                entry["test_helpers"][name] = options[name];
            }
        }

        // Add empty input hashes
        entry["input_hashes"] = Entry::array();

        logger->debug("net_test: tearing down");
        teardown(next_input);

        // Note: annotations must be added before fill_entry because in the
        // latter we will add additional annotations.
        entry["annotations"] = annotations;
        report.fill_entry(entry);
        fixup_entry(entry); // Let drivers possibly fix-up the entry
        // TODO(bassosimone): make sure that this entry contains the report ID
        // which probably is currently not the case.
        logger->emit_event_ex("measurement", nlohmann::json::object({
            {"idx", saved_current_entry},
            {"json_str", entry.dump()},
        }));
        report.write_entry(entry, [=](Error error) {
            if (error) {
                logger->warn("cannot write entry");
                logger->emit_event_ex("failure.measurement_submission", {
                    {"idx", saved_current_entry},
                    {"json_str", entry.dump()},
                    {"failure", error.reason},
                });
                if (not options.get("ignore_write_entry_error", true)) {
                    cb(error);
                    return;
                }
            } else {
                logger->debug("net_test: written entry");
                logger->emit_event_ex("status.measurement_submission", {
                    {"idx", saved_current_entry}
                });
            }
            logger->emit_event_ex("status.measurement_done", {
                {"idx", saved_current_entry}
            });
            reactor->call_soon([=]() {
                run_next_measurement(thread_id, cb, num_entries, current_entry);
            });
        }, logger);
    });
}

void Runnable::geoip_lookup(Callback<> cb) {

    // This is to ensure that when calling multiple times geoip_lookup we
    // always reset the probe_ip, probe_asn and probe_cc values.
    probe_ip = "127.0.0.1";
    probe_asn = "AS0";
    probe_cc = "ZZ";

    auto save_ip = options.get("save_real_probe_ip", false);
    auto save_asn = options.get("save_real_probe_asn", true);
    auto save_cc = options.get("save_real_probe_cc", true);

    // This code block allows the caller to override probe variables
    if (save_ip and options.find("probe_ip") != options.end()) {
        probe_ip = options.at("probe_ip");
        save_ip = false; // We already have it, don't look it up and save it
    }
    if (save_asn and options.find("probe_asn") != options.end()) {
        probe_asn = options.at("probe_asn");
        save_asn = false; // Ditto
    }
    if (save_cc and options.find("probe_cc") != options.end()) {
        probe_cc = options.at("probe_cc");
        save_cc = false; // Ditto
    }

    // No need to perform further lookups if we don't need to save anything
    if (not save_ip and not save_asn and not save_cc) {
        if (probe_ip == "127.0.0.1") {
            logger->warn("Not knowing user_ip means we cannot attempt to "
                         "scrub it from the report");
        }
        /*
         * XXX Passing down the stack the real probe IP to allow
         * specific tests to scrub entries.
         *
         * See also measurement-kit/measurement-kit#1110.
         */
        options["real_probe_ip_"] = probe_ip;
        cb();
        return;
    }

    ip_lookup(
        [=](Error err, std::string ip) {
            if (err) {
                logger->warn("ip_lookup() failed: error code: %d", err.code);
                logger->warn("Not knowing user_ip means we cannot attempt "
                             "to scrub it from the report");
                cb();
                return;
            }
            logger->info("Your public IP address: %s", ip.c_str());
            if (save_ip) {
                logger->debug("saving user's real ip on user's request");
                probe_ip = ip;
            }
            /*
             * XXX Passing down the stack the real probe IP to allow
             * specific tests to scrub entries.
             *
             * See also measurement-kit/measurement-kit#1110.
             */
            options["real_probe_ip_"] = ip;

            auto country_path = options.get("geoip_country_path",
                                            std::string{});
            if (save_cc and country_path != "") {
                auto inst = GeoipCache::thread_local_instance();
                if (!!inst) {
                    auto rv = inst->resolve_country_code(
                            country_path, ip, logger);
                    if (!!rv) {
                        probe_cc = rv.as_value();
                    } else {
                        // Error message already printed
                    }
                } else {
                    logger->warn("cannot access GeoIP instance");
                }
                if (probe_cc != "ZZ") {
                    logger->info("Your country: %s", probe_cc.c_str());
                }
            } else if (country_path == "") {
                logger->warn("geoip_country_path is not set");
            }

            auto asn_path = options.get("geoip_asn_path", std::string{});
            if (save_asn and asn_path != "") {
                auto inst = GeoipCache::thread_local_instance();
                if (!!inst) {
                    auto rv = inst->resolve_asn_full(asn_path, ip, logger);
                    if (!!rv) {
                        auto asninfo = rv.as_value();
                        auto p = asninfo.find(" ");
                        if (p != std::string::npos) {
                            probe_asn = asninfo.substr(0, p);
                            // Note that p + 1 cannot overflow SIZE_MAX since
                            // std::string::npos is SIZE_MAX and we have already
                            // excluded that specific case above.
                            if (asninfo.size() > p + 1) {
                                probe_network_name = asninfo.substr(p + 1);
                            }
                        } else {
                            probe_asn = asninfo;
                        }
                    } else {
                        // Error message already printed
                    }
                } else {
                    logger->warn("cannot access GeoIP instance");
                }
                if (probe_asn != "AS0") {
                    logger->info("Your ISP identifier: %s", probe_asn.c_str());
                }
            } else if (asn_path == "") {
                logger->warn("geoip_asn_path is not set");
            }

            logger->emit_event_ex("status.geoip_lookup", {
                {"probe_asn", probe_asn},
                {"probe_cc", probe_cc},
                {"probe_ip", ip},
                {"probe_network_name", probe_network_name},
            });

            cb();
        },
        options, reactor, logger);
}

void Runnable::open_report(Callback<Error> callback) {
    /*
     * TODO: it would probably more robust to future changes to
     * set these values using a constructor.
     */
    report.test_name = test_name;
    report.test_version = test_version;
    report.test_start_time = test_start_time;

    report.options = options;

    report.probe_ip = probe_ip;
    report.probe_cc = probe_cc;
    report.probe_asn = probe_asn;

    if (output_filepath == "") {
        output_filepath = generate_output_filepath();
    }
    if (!options.get("no_file_report", false)) {
        report.add_reporter(FileReporter::make(output_filepath));
    }
    if (!options.get("no_collector", false)) {
        report.add_reporter(OoniReporter::make(options, reactor, logger));
    }
    report.open(callback);
}

std::string Runnable::generate_output_filepath() {
    int idx = 0;
    std::stringstream filename;
    while (true) {
        filename.str("");
        filename.clear();

        char timestamp[100];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H%M%SZ", &test_start_time);
        filename << "report-" << test_name << "-";
        filename << timestamp << "-" << idx << ".njson";

        std::ifstream output_file(filename.str().c_str());
        // If a file called this way already exists we increment the counter
        if (output_file.good()) {
            output_file.close();
            idx++;
            continue;
        }
        break;
    }
    return filename.str();
}

void Runnable::query_bouncer(Callback<Error> cb) {
    ErrorOr<bool> disable_bouncer = options.get_noexcept(
            "no_bouncer", false);
    if (disable_bouncer.as_error() != NoError()) {
        logger->warn("Invalid 'no_bouncer' option");
        cb(disable_bouncer.as_error());
        return;
    }
    // Note: `use_bouncer` is set for tasks that must not use the bouncer while
    // the setting is to allow users to bypass the bouncer.
    if (!use_bouncer || disable_bouncer.as_value() == true) {
        logger->info("Skipping bouncer");
        cb(NoError());
        return;
    }
    auto bouncer = options.get("bouncer_base_url",
            ooni::bouncer::production_bouncer_url());
    logger->info("Contacting bouncer: %s", bouncer.c_str());
    ooni::bouncer::post_net_tests(
        bouncer, test_name, test_version, test_helpers_bouncer_names(),
        [=](Error error, SharedPtr<BouncerReply> reply) {
            if (error) {
                cb(error);
                return;
            }
            assert(!!reply);
            if (options.find("collector_base_url") == options.end()) {
                auto maybe_collector = reply->get_collector_alternate("https");
                if (!maybe_collector) {
                    logger->warn("no collector found");
                    cb(maybe_collector.as_error());
                    return;
                }
                options["collector_base_url"] = *maybe_collector;
                logger->info("Using discovered collector: %s",
                                        maybe_collector->c_str());
            }
            for (auto th: test_helpers_data) {
                auto maybe_helper = reply->get_test_helper_alternate(
                    th.first, "https"
                );
                if (!maybe_helper) {
                    maybe_helper = reply->get_test_helper(th.first);
                    if (!maybe_helper) {
                        logger->warn("Cannot find alternate or normal helper");
                        continue;
                    }
                }
                logger->info("Bouncer discovered helper for %s: %s",
                     th.first.c_str(), maybe_helper->c_str());
                if (options.find(th.second) != options.end()) {
                    continue;
                }
                logger->info("Using discovered helper as '%s'",
                             th.second.c_str());
                options[th.second] = *maybe_helper;
            }
            cb(NoError());
        },
        options, reactor, logger);
}

void Runnable::begin(Callback<Error> cb) {
    mk::utc_time_now(&test_start_time);
    beginning = mk::time_now();
    query_bouncer([=](Error error) {
        if (error) {
            cb(error);
            return;
        }
        mk::dump_settings(options, "runnable", logger);
        geoip_lookup([=]() {
            resolver_lookup(
                [=](Error error, std::string resolver_ip_) {
                    logger->progress(0.05, "geoip lookup");
                    if (!error) {
                        resolver_ip = resolver_ip_;
                    } else {
                        logger->debug("failed to lookup resolver ip");
                    }
                    open_report([=](Error error) {
                        if (error) {
                            logger->warn("Cannot open report: %s",
                                         error.what());
                            // FALLTHROUGH
                        }
                        logger->progress(0.1, "open report");
                        if (error and
                            not options.get("ignore_open_report_error", true)) {
                            cb(error);
                            return;
                        }
                        logger->set_progress_offset(0.1);
                        logger->set_progress_scale(0.8);

                        error = process_input_filepaths(inputs,
                                needs_input, input_filepaths, probe_cc, options,
                                logger, nullptr, nullptr);
                        if (error) {
                            cb(error);
                            return;
                        }
                        size_t num_entries = inputs.size();

                        // Run `parallelism` measurements in parallel
                        SharedPtr<size_t> current_entry(new size_t(0));
                        mk::parallel(mk::fmap<size_t, Continuation<Error>>(
                                         mk::range<size_t>(
                                             options.get("parallelism", 3)),
                                         [=](size_t thread_id) {
                                             return [=](Callback<Error> cb) {
                                                 run_next_measurement(
                                                     thread_id, cb, num_entries,
                                                     current_entry);
                                             };
                                         }),
                                     cb);

                    });
                },
                options, reactor, logger);
        });
    });
}

void Runnable::end(Callback<Error> cb) {
    logger->set_progress_offset(0.0);
    logger->set_progress_scale(1.0);
    logger->progress(0.95, "ending the test");
    report.close([=](Error err) {
        logger->progress(1.00, "test complete");
        cb(err);
    });
}

std::list<std::string> Runnable::test_helpers_option_names() {
    std::list<std::string> values;
    for (auto &kv : test_helpers_data) {
        values.push_back(kv.second);
    }
    return values;
}

std::list<std::string> Runnable::test_helpers_bouncer_names() {
    std::list<std::string> values;
    for (auto &kv : test_helpers_data) {
        values.push_back(kv.first);
    }
    return values;
}

} // namespace nettests
} // namespace mk
