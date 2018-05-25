// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "src/libmeasurement_kit/engine.hpp"

#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <sstream>
#include <thread>
#include <tuple>
#include <type_traits>

#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/logger.hpp>
#include <measurement_kit/common/nlohmann/json.hpp>
#include <measurement_kit/common/shared_ptr.hpp>
#include <measurement_kit/ffi_macros.h>

#include "src/libmeasurement_kit/common/reactor.hpp"
#include "src/libmeasurement_kit/nettests/runnable.hpp"

namespace mk {
namespace engine {

// # Multi-thread stuff
//
// Comes first because it needs more careful handling.

class Semaphore {
  public:
    Semaphore() = default;

    void acquire() { mutex_.lock(); }

    void release() { mutex_.unlock(); }

    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;
    Semaphore(Semaphore &&) = delete;
    Semaphore &operator=(Semaphore &&) = delete;

    ~Semaphore() = default;

  private:
    std::mutex mutex_;
};

class TaskImpl {
  public:
    std::condition_variable cond;
    std::deque<nlohmann::json> deque;
    std::atomic_bool interrupted{false};
    std::mutex mutex;
    SharedPtr<Reactor> reactor = Reactor::make();
    std::atomic_bool running{false};
    std::thread thread;
};

static void task_run(TaskImpl *pimpl, nlohmann::json &settings);
static nlohmann::json possibly_validate_event(nlohmann::json &&);

static void emit(TaskImpl *pimpl, nlohmann::json &&event) {
    // Perform validation of the event (debug mode only)
    event = possibly_validate_event(std::move(event));
    // Actually emit the event.
    {
        std::unique_lock<std::mutex> _{pimpl->mutex};
        pimpl->deque.push_back(std::move(event));
    }
    pimpl->cond.notify_all(); // more efficient if unlocked
}

Task::Task(nlohmann::json &&settings) {
    pimpl_ = std::make_unique<TaskImpl>();
    // The purpose of `barrier` is to wait in the constructor until the
    // thread for running the test is up and running.
    std::promise<void> barrier;
    std::future<void> started = barrier.get_future();
    pimpl_->thread = std::thread([ //
          this, &barrier, settings = std::move(settings)]() mutable {
        pimpl_->running = true;
        barrier.set_value();
        static Semaphore semaphore;
        {
            nlohmann::json event;
            event["key"] = "status.queued";
            event["value"] = nlohmann::json::object();
            emit(pimpl_.get(), std::move(event));
        }
        semaphore.acquire(); // prevent concurrent tasks
        task_run(pimpl_.get(), settings);
        pimpl_->running = false;
        pimpl_->cond.notify_all(); // tell the readers we're done
        semaphore.release();       // allow another task to run
    });
    started.wait(); // guarantee Task() completes when the thread is running
}

bool Task::is_done() const {
    std::unique_lock<std::mutex> lock{pimpl_->mutex};
    // Rationale: when the task thread terminates, there may be some
    // unread events in queue. We do not consider the task as done until
    // such events have been read and processed by our controller.
    return pimpl_->running == false && pimpl_->deque.empty();
}

void Task::interrupt() {
    // both variables are safe to use in a MT context
    pimpl_->reactor->stop();
    pimpl_->interrupted = true;
}

nlohmann::json Task::wait_for_next_event() {
    std::unique_lock<std::mutex> lock{pimpl_->mutex};
    // purpose: block here until we stop running or we have events to read
    pimpl_->cond.wait(lock, [this]() { //
        return !pimpl_->running || !pimpl_->deque.empty();
    });
    // must be first so we drain the queue before emitting the final null
    if (!pimpl_->deque.empty()) {
        auto rv = std::move(pimpl_->deque.front());
        pimpl_->deque.pop_front();
        return rv;
    }
    assert(!pimpl_->running);
    // Rationale: we used to return `null` when done. But then I figured that
    // this could break people code. So, to ease integrator's life, we now
    // return a dummy event structured exactly like other events.
    return possibly_validate_event(nlohmann::json{
        {"key", "task_terminated"},
        {"value", nlohmann::json::object()},
    });
}

Task::~Task() {
    if (pimpl_->thread.joinable()) {
        pimpl_->thread.join();
    }
}

// # Helpers

static std::tuple<int, bool> log_level_atoi(const std::string &str) {
#define ATOI(value, type_ignored, mandatory_ignored)                           \
    if (str == #value) {                                                       \
        return std::make_tuple(MK_LOG_##value, true);                          \
    }
    MK_ENUM_LOG_LEVELS(ATOI)
#undef ATOI
    return std::make_tuple(0, false);
}

static std::tuple<std::string, bool> log_level_itoa(int n) {
#define ITOA(value, type_ignored, mandatory_ignored)                           \
    if (n == MK_LOG_##value) {                                                 \
        return std::make_tuple(std::string{#value}, true);                     \
    }
    MK_ENUM_LOG_LEVELS(ITOA)
#undef ITOA
    return std::make_tuple(std::string{}, false);
}

static nlohmann::json make_log_event(uint32_t log_level, const char *message) {
    auto log_level_tuple = log_level_itoa(log_level);
    assert(std::get<1>(log_level_tuple));
    const std::string &vs = std::get<0>(log_level_tuple);
    nlohmann::json object;
    object["key"] = "log";
    object["value"]["log_level"] = vs;
    object["value"]["message"] = message;
    return object;
}

static nlohmann::json make_failure_event(const Error &error) {
    nlohmann::json object;
    object["key"] = "failure.startup";
    object["value"]["failure"] = error.reason;
    return object;
}

static bool is_event_valid(const std::string &str) {
    bool rv = false;
    do {
#define CHECK(value, type_ignored, mandatory_ignored)                          \
    if (#value == str) {                                                       \
        rv = true;                                                             \
        break;                                                                 \
    }
        MK_ENUM_EVENTS(CHECK)
#undef CHECK
    } while (0);
    return rv;
}

static nlohmann::json possibly_validate_event(nlohmann::json &&event) {
    // In debug mode, make sure that we're emitting an event that we know
    assert(event.is_object());
    assert(event.count("key") != 0);
    assert(event.at("key").is_string());
    assert(is_event_valid(event.at("key").get<std::string>()));
    assert(event.count("value") != 0);
    assert(event.at("value").is_object());
    return event;
}

static nlohmann::json known_events() {
    nlohmann::json json;
#define ADD(name, type_ignored, mandatory_ignored) json.push_back(#name);
    MK_ENUM_EVENTS(ADD)
#undef ADD
    return json;
}

static std::string known_tasks() {
    nlohmann::json json;
#define ADD(name, type_ignored, mandatory_ignored) json.push_back(#name);
    MK_ENUM_TASKS(ADD)
#undef ADD
    return json.dump();
}

static std::string known_log_level_levels() {
    nlohmann::json json;
#define ADD(name, type_ignored, mandatory_ignored) json.push_back(#name);
    MK_ENUM_LOG_LEVELS(ADD)
#undef ADD
    return json.dump();
}

static std::unique_ptr<nettests::Runnable> make_runnable(const std::string &s) {
    std::unique_ptr<nettests::Runnable> runnable;
#define ATOP(value, type_ignored, mandatory_ignored)                           \
    if (s == #value) {                                                         \
        runnable.reset(new nettests::value##Runnable);                         \
    }
    MK_ENUM_TASKS(ATOP)
#undef ATOP
    return runnable;
}

static void emit_settings_failure(TaskImpl *pimpl, const char *reason) {
    emit(pimpl, make_log_event(MK_LOG_ERR, reason));
    emit(pimpl, make_failure_event(ValueError()));
}

static void emit_settings_warning(TaskImpl *pimpl, const char *reason) {
    emit(pimpl, make_log_event(MK_LOG_WARNING, reason));
}

static bool validate_known_settings_shallow(
        TaskImpl *pimpl, const nlohmann::json &settings) {
    auto rv = true;

#define VALIDATE(name, type, mandatory)                                        \
                                                                               \
    /* Make sure that mandatory settings are present */                        \
    if (!settings.count(#name) && mandatory) {                                 \
        std::stringstream ss;                                                  \
        ss << "missing required setting '" << #name << "' (fyi: '" << #name    \
           << "' should be a " << #type << ")";                                \
        emit_settings_warning(pimpl, ss.str().data());                         \
        rv = false;                                                            \
    }                                                                          \
                                                                               \
    /* Make sure that existing settings have the correct type. */              \
    if (settings.count(#name) && !settings.at(#name).is_##type()) {            \
        std::stringstream ss;                                                  \
        ss << "found setting '" << #name << "' with invalid type (fyi: '"      \
           << #name << "' should be a " << #type << ")";                       \
        emit_settings_warning(pimpl, ss.str().data());                         \
        rv = false;                                                            \
    }

    MK_ENUM_SETTINGS(VALIDATE)
#undef VALIDATE

    return rv;
}

// TODO(bassosimone): decide whether this should instead stop processing.
static void remove_unknown_settings_and_warn(
        TaskImpl *pimpl, nlohmann::json &settings) {
    std::set<std::string> expected;
    std::set<std::string> unexpected;
#define FILL(name, type, mandatory) expected.insert(#name);
    MK_ENUM_SETTINGS(FILL)
#undef FILL
    for (auto it : nlohmann::json::iterator_wrapper(settings)) {
        const auto &key = it.key();
        if (expected.count(key) <= 0) {
            std::stringstream ss;
            ss << "found unknown setting key " << key << " which will be "
               << "ignored by Measurement Kit";
            unexpected.insert(key);
            emit_settings_warning(pimpl, ss.str().data());
        }
    }
    for (auto &s : unexpected) {
        settings.erase(s);
    }
}

// # Run task

static void task_run(TaskImpl *pimpl, nlohmann::json &settings) {

    // make sure that `settings` is an object
    if (!settings.is_object()) {
        std::stringstream ss;
        ss << "invalid `settings` type: the `settings` JSON that you pass me "
            << "should be a JSON object (i.e. '{\"type\": \"Ndt\"}') but "
            << "instead you passed me this: '" << settings.dump() << "'";
        emit_settings_failure(pimpl, ss.str().data());
        return;
    }

    // Make sure that the toplevel settings are okay, remove unknown ones, so
    // there cannot be code below attempting to access settings that are not
    // specified also inside of the <engine.h> header file.
    if (!validate_known_settings_shallow(pimpl, settings)) {
        emit_settings_failure(pimpl, "failed to validate settings");
        return;
    }
    remove_unknown_settings_and_warn(pimpl, settings);

    // extract and process `name`
    auto runnable = make_runnable(settings.at("name").get<std::string>());
    if (!runnable) {
        std::stringstream ss;
        ss << "unknown task name '" << settings.at("name").get<std::string>()
            << "' (fyi: known tasks are: " << known_tasks() << ")";
        emit_settings_failure(pimpl, ss.str().data());
        return;
    }
    runnable->reactor = pimpl->reactor; // default is nullptr, we must set it

    // extract and process `options`
    if (settings.count("options") != 0) {
        auto &options = settings.at("options");
        // TODO(bassosimone): enumerate all possible options. Make sure we
        // check their type, warn about unknown options (but don't remove them
        // for now, as it will take time to map all of them).
        for (auto it : nlohmann::json::iterator_wrapper(options)) {
            const auto &key = it.key();
            auto &value = it.value();
            if (value.is_string()) {
                const auto &v = value.get<std::string>();
                if (v == "") {
                    // TODO(bassosimone): modify the Scalar class so that it
                    // does not break if we set the value as an empty string and
                    // we then attempt to extract a string. This is currently a
                    // nasty bug that, without this workaround, makes the life
                    // of people integrating MK quite annoying.
                    continue;
                }
                // Using emplace() as a workaround for bug #1550.
                runnable->options.emplace(key, v);
            } else if (value.is_number_integer()) {
                auto v = value.get<int64_t>();
                runnable->options[key] = v;
            } else if (value.is_number_float()) {
                auto v = value.get<double>();
                runnable->options[key] = v;
            } else {
                std::stringstream ss;
                ss << "Found option '" << key << "' to have an invalid type"
                    << " (fyi: valid types are: int, double, string)";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
        }
    }

    // extract and process `annotations`
    if (settings.count("annotations") != 0) {
        auto &annotations = settings.at("annotations");
        for (auto it : nlohmann::json::iterator_wrapper(annotations)) {
            const auto &key = it.key();
            auto &value = it.value();
            // TODO(bassosimone): make sure that we preserve the _type_ of
            // the annotation in the final report rather than converting such
            // type into a string, which is currently what we do.
            if (value.is_string()) {
                runnable->annotations[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                int64_t intvalue = value.get<int64_t>();
                runnable->annotations[key] = std::to_string(intvalue);
            } else if (value.is_number_float()) {
                double doublevalue = value.get<double>();
                runnable->annotations[key] = std::to_string(doublevalue);
            } else {
                std::stringstream ss;
                ss << "Found annotation '" << key << "' to have an invalid type"
                    << " (fyi: valid types are: int, double, string)";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
        }
    }

    // extract and process `inputs`
    if (settings.count("inputs") != 0) {
        for (auto &value : settings.at("inputs")) {
            if (value.is_string()) {
                runnable->inputs.push_back(value.get<std::string>());
            } else {
                std::stringstream ss;
                ss << "Found input '" << value << "' to have an invalid type"
                    << " (fyi: values inside 'inputs' must be strings)";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
        }
    }

    // extract and process `input_filepaths`
    if (settings.count("input_filepaths") != 0) {
        for (auto &value : settings.at("input_filepaths")) {
            if (value.is_string()) {
                runnable->input_filepaths.push_back(value.get<std::string>());
            } else {
                std::stringstream ss;
                ss << "Found input_filepath '" << value << "' to have an "
                   << "invalid type (fyi: values inside 'input_filepaths' "
                   << "must be strings)";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
        }
    }

    // extract and process `log_filepath`
    if (settings.count("log_filepath") != 0) {
        auto &value = settings.at("log_filepath");
        runnable->logger->set_logfile(value.get<std::string>());
    }

    // extract and process `output_filepath`
    if (settings.count("output_filepath") != 0) {
        auto &value = settings.at("output_filepath");
        runnable->output_filepath = value.get<std::string>();
    }

    // extract and process `log_level`
    {
        uint32_t log_level = MK_LOG_WARNING;
        if (settings.count("log_level") != 0) {
            auto log_level_string = settings.at("log_level").get<std::string>();
            auto log_level_tuple = log_level_atoi(log_level_string);
            bool okay = std::get<1>(log_level_tuple);
            if (!okay) {
                std::stringstream ss;
                ss << "Unknown log_level level '" << log_level_string << "' "
                    << "(fyi: known log_level levels are: " <<
                    known_log_level_levels() << ")";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
            log_level = std::get<0>(log_level_tuple);
        }
        runnable->logger->set_verbosity(log_level);
    }

    // Mask out events that are user-disabled.
    std::set<std::string> enabled_events = known_events();
    if (settings.count("disabled_events") != 0) {
        for (auto &entry : settings.at("disabled_events")) {
            if (!entry.is_string()) {
                std::stringstream ss;
                ss << "Found invalid entry inside of disabled_events that "
                  << "has value equal to <" << entry.dump() << "> (fyi: all "
                  << "the entries in disabled_events must be strings";
                emit_settings_failure(pimpl, ss.str().data());
                return;
            }
            std::string s = entry.get<std::string>();
            if (!is_event_valid(s)) {
                std::stringstream ss;
                ss << "Found unknown event inside of disabled_events with "
                   << "name '" << s << "' (fyi: all valid events are: "
                   << known_events().dump() << "). Measurement Kit is going "
                   << "to ignore this invalid event and continue";
                emit_settings_warning(pimpl, ss.str().data());
                continue;
            }
            enabled_events.erase(s);
        }
    }

    // see whether 'log' is enabled
    if (enabled_events.count("log") != 0) {
        runnable->logger->on_log([pimpl](uint32_t log_level, const char *line) {
            if ((log_level & ~MK_LOG_VERBOSITY_MASK) != 0) {
                return; // mask out non-logging events
            }
            emit(pimpl, make_log_event(log_level, line));
        });
        enabled_events.erase("log"); // we have consumed this event key
    } else {
        runnable->logger->on_log(nullptr);
    }

    // intercept and route all the other possible events
    for (auto &event : enabled_events) {
        runnable->logger->on_event_ex(event, [pimpl](nlohmann::json &&event) {
            emit(pimpl, std::move(event));
        });
    }

    runnable->logger->emit_event_ex("status.started", nlohmann::json::object());

    // start the task (reactor and interrupted are MT safe)
    Error error = GenericError();
    pimpl->reactor->run_with_initial_event([&]() {
        if (pimpl->interrupted) {
            return; // allow for early interruption
        }
        runnable->begin([&](Error err) {
            error = err;
            runnable->end([&](Error err) {
                if (error != NoError()) {
                    error = err;
                }
            });
        });
    });

    DataUsage du;
    runnable->reactor->with_current_data_usage([&](DataUsage &x) {
        du = x;
    });
    runnable->logger->emit_event_ex("status.end", {
        {"downloaded_kb", du.down / 1024.0},
        {"failure", error.reason},
        {"uploaded_kb", du.up / 1024.0},
    });
}

} // namespace engine
} // namespace mk
