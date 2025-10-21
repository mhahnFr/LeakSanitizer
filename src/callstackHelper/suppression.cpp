/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#define LCS_USE_UNSAFE_OPTIMIZATION 1

#include "suppression.hpp"

#include <algorithm>
#include <callstack_internals.h>

#include "../suppression/firstPartyLibrary.hpp"

namespace lsan::callstackHelper {
/**
 * Returns whether the given range or regex matches the given callstack frame.
 *
 * @param supp the regex or range
 * @param frame the callstack frame
 * @param address the address of the represented function call
 * @return whether the callstack frame was matched
 */
static inline auto match(const suppression::Suppression::RangeOrRegexType& supp, const callstack_frame* frame, const uintptr_t address) -> bool {
    if (supp.first == suppression::Suppression::Type::range) {
        const auto& [begin, end] = std::get<suppression::Suppression::RangeType>(supp.second);
        return address >= begin && address <= begin + end;
    }
    const auto& suppressions = std::get<suppression::Suppression::RegexType>(supp.second);
    const char* binaryFile = frame->binaryFile;
    [[unlikely]] if (binaryFile == nullptr) {
        binaryFile = "";
    }
    return std::ranges::any_of(suppressions, [&](const std::regex& regex) {
        return frame->binaryFileIsSelf
            || (std::regex_match("LSAN_SYSTEM_LIBRARIES", regex) && suppression::isFirstParty(binaryFile, !callstack_autoClearCaches))
            || std::regex_match(binaryFile, regex);
    });
}

auto isSuppressed(const suppression::Suppression& suppression, lcs::callstack& callstack) -> bool {
    const callstack_frame* binaries = nullptr;
    if (suppression.hasRegexes) {
        binaries = callstack_autoClearCaches ? callstack_getBinaries(callstack) : callstack_getBinariesCached(callstack);
        [[unlikely]] if (binaries == nullptr) {
            return false;
        }
    }
    for (std::size_t i = 0; i + suppression.topCallstack.size() <= callstack->backtraceSize; ++i) {
        auto matched { false };

        for (std::size_t j = 0, k = i; j < suppression.topCallstack.size() && k < callstack->backtraceSize; ) {
            const auto& address = uintptr_t(callstack->backtrace[k]);
            const auto& hereMatch = match(suppression.topCallstack[j], binaries + k, address);
            if (suppression.topCallstack[j].first == suppression::Suppression::Type::regex) {
                auto nextMatch = false;
                if (j + 1 < suppression.topCallstack.size()) {
                    nextMatch = match(suppression.topCallstack[j + 1], binaries + k, address);
                }

                matched = hereMatch;
                if (nextMatch) {
                    ++j;
                    continue;
                }
                ++k;
                if (hereMatch) {
                    continue;
                }
                return false;
            } else {
                if (hereMatch) {
                    matched = true;
                } else {
                    matched = false;
                    break;
                }
                ++j;
                ++k;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}
}