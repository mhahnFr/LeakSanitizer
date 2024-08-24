/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
 *
 * This file is part of the LeakSanitizer.
 *
 * The LeakSanitizer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The LeakSanitizer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with the
 * LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>

#include <lsan_internals.h>

#include <callstack_internals.h>

#include "LeakSani.hpp"

#include "bytePrinter.hpp"
#include "formatter.hpp"
#include "lsanMisc.hpp"
#include "TLSTracker.hpp"
#include "callstacks/callstackHelper.hpp"
#include "crashWarner/exceptionHandler.hpp"
#include "signals/signals.hpp"
#include "signals/signalHandlers.hpp"

namespace lsan {
std::atomic_bool LSan::finished = false;
std::atomic_bool LSan::preventDealloc = false;

auto LSan::generateRegex(const char * regex) -> std::optional<std::regex> {
    if (regex == nullptr || *regex == '\0') {
        return std::nullopt;
    }
    
    try {
        return std::regex(regex);
    } catch (std::regex_error & e) {
        userRegexError = e.what();
        return std::nullopt;
    }
}

/**
 * If the given pointer is a TLSTracker, it is deleted and the thread-local
 * value is set to point to the global tracker instance.
 *
 * @param value the thread-local value
 */
static inline void destroySaniKey(void* value) {
    auto& globalInstance = getInstance();
    if (value != std::addressof(globalInstance)) {
        pthread_setspecific(globalInstance.saniKey, std::addressof(globalInstance));
        auto tracker = static_cast<TLSTracker*>(value);
        if (!globalInstance.preventDealloc) {
            std::lock_guard lock(globalInstance.mutex);
            auto ignore = globalInstance.ignoreMalloc;
            globalInstance.ignoreMalloc = true;
            delete tracker;
            globalInstance.ignoreMalloc = ignore;
        } else {
            tracker->needsDealloc = true;
        }
    }
}

/**
 * @brief Creates and returns a thread-local storage key with the function
 * `destroySaniKey` as destructor.
 *
 * Throws an exception if no key could be created.
 *
 * @return the thread-local storage key
 */
static inline auto createSaniKey() -> pthread_key_t {
    pthread_key_t key;
    if (pthread_key_create(&key, destroySaniKey) != 0) {
        throw std::runtime_error("Could not create TLS key!");
    }
    return key;
}

LSan::LSan(): saniKey(createSaniKey()) {
    atexit(exitHook);

    signals::registerFunction(signals::handlers::stats, SIGUSR1);
    
    signals::registerFunction(signals::asHandler(signals::handlers::callstack), SIGUSR2, false);
    
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGSEGV);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGABRT);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGTERM);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGALRM);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGPIPE);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGFPE);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGILL);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGQUIT);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGHUP);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGBUS);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGXFSZ);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGXCPU);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGSYS);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGVTALRM);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGPROF);
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGTRAP);

#if defined(__APPLE__) || defined(SIGEMT)
    signals::registerFunction(signals::asHandler(signals::handlers::crashWithTrace), SIGEMT);
#endif

    std::set_terminate(exceptionHandler);
}

LSan::~LSan() {
    for (auto tracker : tlsTrackers) {
        if (tracker->needsDealloc) {
            delete tracker;
        }
    }
    preventDealloc = false;
}

auto LSan::copyTrackerList() -> decltype(tlsTrackers) {
    std::lock_guard lock { tlsTrackerMutex };

    return tlsTrackers;
}

void LSan::finish() {
    preventDealloc = true;
    finished = true;
    {
        std::lock_guard lock { mutex };
        ignoreMalloc = true;
    }

    auto trackers = copyTrackerList();
    for (auto tracker : trackers) {
        tracker->finish();
    }
}

void LSan::registerTracker(ATracker* tracker) {
    std::lock_guard lock1 { mutex };
    std::lock_guard lock { tlsTrackerMutex };

    auto ignore = ignoreMalloc;
    ignoreMalloc = true;
    tlsTrackers.insert(tracker);
    ignoreMalloc = ignore;
}

void LSan::deregisterTracker(ATracker* tracker) {
    std::lock_guard lock1 { mutex };
    std::lock_guard lock { tlsTrackerMutex };

    auto ignore = ignoreMalloc;
    ignoreMalloc = true;
    tlsTrackers.erase(tracker);
    ignoreMalloc = ignore;
}

void LSan::absorbLeaks(PoolMap<const void *const, MallocInfo>&& leaks) {
    std::lock_guard lock { mutex };
    std::lock_guard lock1 { infoMutex };

    auto ignore = ignoreMalloc;
    ignoreMalloc = true;
    infos.get_allocator().merge(leaks.get_allocator());
    infos.merge(std::move(leaks));
    ignoreMalloc = ignore;
}

// FIXME: Though unlikely, the invalidly freed record ref can become invalid throughout this process
auto LSan::removeMalloc(ATracker* tracker, void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    const auto& result = maybeRemoveMalloc(pointer);
    std::pair<bool, std::optional<MallocInfo::CRef>> tmp { false, std::nullopt };
    if (!result.first) {
        std::lock_guard lock { tlsTrackerMutex };
        for (auto element : tlsTrackers) {
            if (element == tracker) continue;

            const auto& result = element->maybeRemoveMalloc(pointer);
            if (result.first) {
                return result;
            }
            if (!tmp.second || (tmp.second && result.second && result.second->get().isMoreRecent(tmp.second->get()))) {
                tmp = std::move(result);
            }
        }
    }
    if (!result.first) {
        if (result.second && tmp.second) {
            return result.second->get().isMoreRecent(tmp.second->get()) ? result : tmp;
        }
        return result.second ? result : tmp;
    }
    return result;
}

auto LSan::removeMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    return removeMalloc(nullptr, pointer);
}

auto LSan::maybeRemoveMalloc(void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(pointer);
    if (it == infos.end()) {
        return std::make_pair(false, std::nullopt);
    }
    if (it->second.deleted) {
        return std::make_pair(false, std::ref(it->second));
    }
    if (__lsan_statsActive) {
        stats -= it->second;
    }
    if (__lsan_statsActive) {
        it->second.markDeleted();
    } else {
        infos.erase(it);
    }
    return std::make_pair(true, std::nullopt);
}

void LSan::changeMalloc(ATracker* tracker, MallocInfo&& info) {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(info.pointer);
    if (it == infos.end()) {
        std::lock_guard tlsLock { tlsTrackerMutex };
        for (auto element : tlsTrackers) {
            if (element == tracker) continue;

            if (element->maybeChangeMalloc(info)) {
                return;
            }
        }
        return;
    }
    if (__lsan_statsActive) {
        stats.replaceMalloc(it->second.size, info.size);
    }
    infos.insert_or_assign(info.pointer, info);
}

void LSan::changeMalloc(MallocInfo&& info) {
    changeMalloc(nullptr, std::move(info));
}

auto LSan::getTotalAllocatedBytes() -> std::size_t {
    std::lock_guard lock(infoMutex);
    
    std::size_t ret = 0;
    for (const auto & [ptr, info] : infos) {
        ret += info.size;
    }
    return ret;
}

/**
 * Prints the callstack size exceeded hint onto the given output stream.
 *
 * @param stream the output stream to print to
 * @return the given output stream
 */
static inline auto printCallstackSizeExceeded(std::ostream & stream) -> std::ostream & {
    using formatter::Style;
    
    stream << "Hint:" << formatter::get<Style::GREYED>
           << formatter::format<Style::ITALIC>(" to see longer callstacks, increase the value of ")
           << formatter::clear<Style::GREYED> << "LSAN_CALLSTACK_SIZE" << formatter::get<Style::GREYED>
           << " (__lsan_callstackSize)" << formatter::format<Style::ITALIC>(" (currently ")
           << formatter::clear<Style::GREYED> << __lsan_callstackSize
           << formatter::format<Style::ITALIC, Style::GREYED>(").") << std::endl << std::endl;
    
    return stream;
}

auto LSan::maybeHintCallstackSize(std::ostream & out) const -> std::ostream & {
    if (callstackSizeExceeded) {
        out << printCallstackSizeExceeded;
    }
    return out;
}

/**
 * Prints a deprecation notice using the given information.
 *
 * @param out the output stream to print to
 * @param envName the name of the variable in the environment
 * @param apiName the name of the variable in the C API
 * @param message the deprecation message
 */
static inline void printDeprecation(      std::ostream & out,
                                    const std::string &  envName,
                                    const std::string &  apiName,
                                    const std::string &  message) {
    using formatter::Style;
    
    out << std::endl << formatter::format<Style::RED>(formatter::formatString<Style::BOLD>(envName) + " ("
                                                      + formatter::formatString<Style::ITALIC>(apiName) + ") " + message + "!")
        << std::endl;
}

/**
 * This function prints the deprecation warnings for deprecated variables
 * found in the environment.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
static inline auto maybeShowDeprecationWarnings(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    if (has("LSAN_PRINT_STATS_ON_EXIT")) {
        printDeprecation(out,
                         "LSAN_PRINT_STATS_ON_EXIT",
                         "__lsan_printStatsOnExit",
                         "is no longer supported and " + formatter::formatString<Style::BOLD>("deprecated since version 1.7"));
    }
    if (has("LSAN_PRINT_LICENSE")) {
        printDeprecation(out,
                         "LSAN_PRINT_LICENSE",
                         "__lsan_printLicense",
                         "is no longer supported and " + formatter::formatString<Style::BOLD>("deprecated since version 1.8"));
    }
    if (has("LSAN_PRINT_WEBSITE")) {
        printDeprecation(out,
                         "LSAN_PRINT_WEBSITE",
                         "__lsan_printWebsite",
                         "is no longer supported and " + formatter::formatString<Style::BOLD>("deprecated since version 1.8"));
    }
    return out;
}

auto operator<<(std::ostream& stream, LSan& self) -> std::ostream& {
    using formatter::Style;
    
    std::lock_guard lock(self.infoMutex);

    callstack_autoClearCaches = false;
    std::size_t i     = 0,
                j     = 0,
                bytes = 0,
                count = 0,
                total = self.infos.size();
    for (auto & [ptr, info] : self.infos) {
        if (isATTY()) {
            char buffer[7] {};
            std::snprintf(buffer, 7, "%05.2f", static_cast<double>(j) / total * 100);
            stream << "\rCollecting the leaks: " << formatter::format<Style::BOLD>(buffer) << " %";
        }
        if (!info.deleted && callstackHelper::getCallstackType(info.createdCallstack) == callstackHelper::CallstackType::USER) {
            ++count;
            bytes += info.size;
            if (i < __lsan_leakCount) {
                if (isATTY()) {
                    stream << "\r                                    \r";
                }
                stream << info << std::endl;
                ++i;
            }
        }
        ++j;
    }
    if (isATTY()) {
        stream << "\r                                    \r";
    }
    if (self.callstackSizeExceeded) {
        stream << printCallstackSizeExceeded;
        self.callstackSizeExceeded = false;
    }
    if (i < count) {
        stream << std::endl << formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(count - i) + " more...") << std::endl << std::endl
               << "Hint:" << formatter::format<Style::GREYED, Style::ITALIC>(" to see more, increase the value of ")
               << "LSAN_LEAK_COUNT" << formatter::get<Style::GREYED> << " (__lsan_leakCount)"
               << formatter::format<Style::ITALIC>(" (currently ") << formatter::clear<Style::GREYED>
               << __lsan_leakCount << formatter::format<Style::ITALIC, Style::GREYED>(").") << std::endl << std::endl;
    }
    
    if (count == 0) {
        stream << formatter::format<Style::ITALIC>(self.infos.empty() ? "No leaks possible." : "No leaks detected.") << std::endl;
    }
    if (__lsan_relativePaths && count > 0) {
        stream << std::endl << printWorkingDirectory;
    }
    stream << maybeShowDeprecationWarnings;
    if (self.userRegexError.has_value()) {
        stream << std::endl << formatter::get<Style::RED>
               << formatter::format<Style::BOLD>("LSAN_FIRST_PARTY_REGEX") << " ("
               << formatter::format<Style::ITALIC>("__lsan_firstPartyRegex") << ") "
               << formatter::format<Style::BOLD>("ignored: ")
               << formatter::format<Style::ITALIC, Style::BOLD>("\"" + self.userRegexError.value() + "\"")
               << formatter::clear<Style::RED> << std::endl;
    }
    
    if (count > 0) {
        stream << std::endl << formatter::format<Style::BOLD>("Summary: ");
        if (i == __lsan_leakCount && i < count) {
            stream << "showing " << formatter::format<Style::ITALIC>(std::to_string(i)) << " of ";
        }
        stream << formatter::format<Style::BOLD>(std::to_string(count)) << " leaks, "
               << formatter::format<Style::BOLD>(bytesToString(bytes)) << " lost.";
        stream << std::endl;
    }
    
    callstack_clearCaches();
    callstack_autoClearCaches = true;
    
#ifdef BENCHMARK
    stream << std::endl << timing::printTimings << std::endl;
#endif
    return stream;
}
}
