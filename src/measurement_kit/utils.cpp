// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include <measurement_kit/common/json.hpp>

#include <inttypes.h>

#include "../measurement_kit/cmdline.hpp"

BaseTest &common_init(std::list<Callback<BaseTest &>> il, BaseTest &test) {
    test
        .set_verbosity(MK_LOG_INFO)
        .set_option("geoip_country_path", "country.mmdb")
        .set_option("geoip_asn_path", "asn.mmdb")
        .on_progress([](double progress, std::string msg) {
            printf("%.0f%%: %s\n", 100.0 * progress, msg.c_str());
            fflush(stdout);
        })
        .on_overall_data_usage([](DataUsage du) {
            // See s/l/n/messages.cpp
            printf("Overall data usage (bytes): %s down - %s up\n",
                   std::to_string(du.down).c_str(),
                   std::to_string(du.up).c_str());
        })
        .on_log([](uint32_t level, const char *message) {
            if (level <= MK_LOG_WARNING) {
                fprintf(stderr, "[!] ");
            } else if (level >= MK_LOG_DEBUG) {
                fprintf(stderr, "[D] ");
            }
            fprintf(stderr, "%s\n", message);
        });
    for (auto fn : il) {
        fn(test);
    }
    return test;
}

BaseTest &ndt_init(std::list<Callback<BaseTest &>> il, BaseTest &t) {
    return common_init(il, t).on_event([](const char * /*unused*/) {
        // NOTHING: this is done just to suppress printing events as JSONs
        // as part of logging, which is annoying from the CLI.
    });
}

std::vector<option> as_long_options(const OptionSpec *os) {
    std::vector<option> ret;
    for (auto sos = os; sos->short_name != 0; ++sos) {
        option op = {};
        op.name = sos->long_name;
        op.has_arg = (sos->requires_argument) ? required_argument : no_argument;
        op.val = sos->short_name;
        ret.push_back(op);
    }
    return ret;
}

std::string as_getopt_string(const OptionSpec *os) {
    /*
     * Note: the leading `+` tells GNU getopt() to avoid reordering
     * options, which allows us to have parse global options, followed
     * by a command, followed by specific options.
     */
    std::string ret = "+";
    for (auto sos = os; sos->short_name != 0; ++sos) {
        if (sos->short_name > 255) {
            continue;
        }
        ret += static_cast<char>(sos->short_name);
        if (sos->requires_argument) {
            ret += ":";
        }
    }
    return ret;
}

std::string as_available_options_string(const OptionSpec *os) {
    std::string ret = "Options:\n";
    for (auto sos = os; sos->short_name != 0; ++sos) {
        std::string x;
        if (sos->short_name <= 255) {
            x += "  -";
            x += static_cast<char>(sos->short_name);
            x += ", ";
        } else {
            x += "      ";
        }
        x += "--";
        x += sos->long_name;
        if (sos->argument_name != nullptr) {
            x += "=";
            x += sos->argument_name;
        }
        while (x.size() < 32) {
            x += " ";
        }
        x += sos->description;
        x += "\n";
        ret += x;
    }
    ret += "\n";
    return ret;
}
