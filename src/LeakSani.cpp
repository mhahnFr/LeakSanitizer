/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2023  mhahnFr and contributors
 *
 * This file is part of the LeakSanitizer. This library is free software:
 * you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dlfcn.h>

#include <algorithm>
#include <iostream>

#include "LeakSani.hpp"

#include "bytePrinter.hpp"
#include "formatter.hpp"
#include "lsanMisc.hpp"
#include "signalHandlers.hpp"
#include "callstacks/callstackHelper.hpp"

#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

#include "../CallstackLibrary/include/callstack_internals.h"

namespace lsan {
/**
 * Returns an optional containing the runtime name of this library.
 *
 * @return the runtime name of this library if available
 */
static inline auto lsanName() -> std::optional<const std::string> {
    Dl_info info;
    if (!dladdr(reinterpret_cast<const void *>(&lsanName), &info)) {
        return std::nullopt;
    }
    
    return info.dli_fname;
}

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

LSan::LSan(): libName(lsanName().value()) {
    atexit(reinterpret_cast<void (*)()>(exitHook));
    struct sigaction s{};
    s.sa_sigaction = crashHandler;
    sigaction(SIGSEGV, &s, nullptr);
    sigaction(SIGBUS, &s, nullptr);
    signal(SIGUSR1, statsSignal);
    signal(SIGUSR2, callstackSignal);
}

auto LSan::removeMalloc(void * pointer, void * omitAddress) -> MallocInfoRemoved {
    std::lock_guard lock(infoMutex);
    
    auto it = infos.find(pointer);
    if (it == infos.end()) {
        return MallocInfoRemoved(false, std::nullopt);
    } else if (it->second.isDeleted()) {
        return MallocInfoRemoved(false, it->second);
    }
    if (__lsan_statsActive) {
        stats -= it->second;
        it->second.setDeleted(true, omitAddress);
    } else {
        infos.erase(it);
    }
    return MallocInfoRemoved(true, std::nullopt);
}

auto LSan::changeMalloc(const MallocInfo & info) -> bool {
    std::lock_guard lock(infoMutex);

    auto it = infos.find(info.getPointer());
    if (it == infos.end()) {
        return false;
    }
    if (__lsan_statsActive) {
        if (it->second.getPointer() != info.getPointer()) {
            stats -= it->second;
            stats += info;
        } else {
            stats.replaceMalloc(it->second.getSize(), info.getSize());
        }
        it->second.setDeleted(true);
    }
    infos.insert_or_assign(info.getPointer(), info);
    return true;
}

void LSan::addMalloc(MallocInfo && info) {
    std::lock_guard lock(infoMutex);
    
    if (__lsan_statsActive) {
        stats += info;
    }
    
    infos.insert_or_assign(info.getPointer(), info);
}

auto LSan::getTotalAllocatedBytes() -> std::size_t {
    std::lock_guard lock(infoMutex);
    
    std::size_t ret = 0;
    for (const auto & [ptr, info] : infos) {
        ret += info.getSize();
    }
    return ret;
}

/**
 * Signal handler used to exit this sanitizer and to reset the visibility
 * of the terminal cursor.
 *
 * @param signal the signal code, passed to the exit function
 */
static inline void resetCursorSignal(int signal) {
    (__lsan_printCout ? std::cout : std::cerr) << "\033[?25h";
    std::_Exit(signal);
}

auto LSan::getLeakNumbers() -> std::tuple<std::size_t, std::size_t, std::forward_list<std::reference_wrapper<const MallocInfo>>> {
    std::forward_list<std::reference_wrapper<const MallocInfo>> buffer;
    std::size_t count = 0,
                bytes = 0,
                i     = 0,
                total = infos.size();
    
    auto & out = __lsan_printCout ? std::cout : std::cerr;
    if (__lsan_printFormatted) {
        out << "\033[?25l";
        std::signal(SIGINT,  resetCursorSignal);
        std::signal(SIGTERM, resetCursorSignal);
    }
    for (auto & [ptr, info] : infos) {
        if (__lsan_printFormatted) {
            char buffer[7] {};
            std::snprintf(buffer, 7, "%05.2f", static_cast<double>(i) / total * 100);
            out << "\rCollecting the leaks: " << formatter::format<formatter::Style::BOLD>(buffer) << " %";
        }
        
        if (!info.isDeleted() && callstackHelper::getCallstackType(info.getCreatedCallstack()) == callstackHelper::CallstackType::USER) {
            ++count;
            bytes += info.getSize();
            buffer.push_front(info);
        }
        ++i;
    }
    if (__lsan_printFormatted) {
        out << "\r\033[?25h                                    \r";
        std::signal(SIGINT,  SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);
    }
    return std::make_tuple(count, bytes, buffer);
}

/**
 * Prints the callstack size exceeded hint onto the given output stream.
 *
 * @param stream the output stream to print to
 */
static inline void printCallstackSizeExceeded(std::ostream & stream) {
    using formatter::Style;
    
    stream << "Hint:" << formatter::get<Style::GREYED>
           << formatter::format<Style::ITALIC>(" to see longer callstacks, increase the value of ")
           << formatter::clear<Style::GREYED> << "LSAN_CALLSTACK_SIZE" << formatter::get<Style::GREYED>
           << " (__lsan_callstackSize)" << formatter::format<Style::ITALIC>(" (currently ")
           << formatter::clear<Style::GREYED> << __lsan_callstackSize
           << formatter::format<Style::ITALIC, Style::GREYED>(").") << std::endl << std::endl;
}

void LSan::maybeHintCallstackSize(std::ostream & out) const {
    if (callstackSizeExceeded) {
        printCallstackSizeExceeded(out);
    }
}

std::ostream & operator<<(std::ostream & stream, LSan & self) {
    using formatter::Style;
    
    std::lock_guard lock(self.infoMutex);
    
    const auto & [count, bytes, leaks] = self.getLeakNumbers();
    if (count == 0) {
        stream << formatter::format<Style::ITALIC>(self.infos.empty() ? "No leaks possible." : "No leaks detected.") << std::endl;
        return stream;
    }
    
    stream << formatter::get<Style::ITALIC>
           << count << " leaks total, " << bytesToString(bytes) << " total" << std::endl << std::endl;
    callstack_autoClearCaches = false;
    std::size_t i = 0;
    for (auto & leakInfo : leaks) {
        if (i + 1 > __lsan_leakCount) {
            if (self.callstackSizeExceeded) {
                printCallstackSizeExceeded(stream);
                self.callstackSizeExceeded = false;
            }
            stream << std::endl << formatter::format<Style::UNDERLINED, Style::ITALIC>("And " + std::to_string(count - i) + " more...") << std::endl << std::endl
                   << "Hint:" << formatter::format<Style::GREYED, Style::ITALIC>(" to see more, increase the value of ")
                   << "LSAN_LEAK_COUNT" << formatter::get<Style::GREYED> << " (__lsan_leakCount)"
                   << formatter::format<Style::ITALIC>(" (currently ") << formatter::clear<Style::GREYED>
                   << __lsan_leakCount << formatter::format<Style::ITALIC, Style::GREYED>(").") << std::endl;
            break;
        }
        stream << leakInfo << std::endl;
        ++i;
    }
    if (self.callstackSizeExceeded) {
        printCallstackSizeExceeded(stream);
        self.callstackSizeExceeded = false;
    }
    
    if (self.userRegexError.has_value()) {
        stream << std::endl << formatter::get<Style::RED>
               << formatter::format<Style::BOLD>("LSAN_FIRST_PARTY_REGEX") << " ("
               << formatter::format<Style::ITALIC>("__lsan_firstPartyRegex") << ") "
               << formatter::format<Style::BOLD>("ignored: ")
               << formatter::format<Style::ITALIC, Style::BOLD>("\"" + self.userRegexError.value() + "\"")
               << formatter::clear<Style::RED> << std::endl;
    }
    
    stream << std::endl << formatter::format<Style::BOLD>("Summary: ");
    if (i == __lsan_leakCount) {
        stream << "showing " << formatter::format<Style::ITALIC>(std::to_string(i)) << " of ";
    }
    stream << formatter::format<Style::BOLD>(std::to_string(count)) << " leaks, "
           << formatter::format<Style::BOLD>(bytesToString(bytes)) << " lost.";
    stream << std::endl;
    
    stream << formatter::clear<Style::ITALIC>;
    callstack_clearCaches();
    callstack_autoClearCaches = true;
    return stream;
}
}
