/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr and contributors
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

#include <cassert>

#include <dlfcn.h>

#include <algorithm>
#include <stack>

#include "LeakSani.hpp"

#include "bytePrinter.hpp"
#include "formatter.hpp"
#include "lsanMisc.hpp"
#include "callstacks/callstackHelper.hpp"
#include "signals/signals.hpp"
#include "signals/signalHandlers.hpp"

#include "../include/lsan_internals.h"
#include "../include/lsan_stats.h"

#include "../CallstackLibrary/include/callstack_internals.h"

#if defined(__x86_64__) || defined(__i386__)
 #define LSAN_CAN_WALK_STACK 1
 #define LSAN_STACK_X86
#elif defined(__arm64__) || defined(__arm__)
 #define LSAN_CAN_WALK_STACK 0
 #define LSAN_STACK_ARM
#else
 #define LSAN_CAN_WALK_STACK 0
 #define LSAN_STACK_UNKNOWN
#endif

#ifdef __clang__
 #define LSAN_DIAGNOSTIC_PUSH _Pragma("clang diagnostic push")
 #define LSAN_DIAGNOSTIC_POP _Pragma("clang diagnostic pop")
 #define LSAN_IGNORE_FRAME_ADDRESS _Pragma("clang diagnostic ignored \"-Wframe-address\"")

#elif defined(__GNUG__)
 #define LSAN_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
 #define LSAN_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")
 #define LSAN_IGNORE_FRAME_ADDRESS _Pragma("GCC diagnostic ignored \"-Wframe-address\"")

#else
 #define LSAN_DIAGNOSTIC_PUSH
 #define LSAN_DIAGNSOTIC_POP
#endif

#include <mach-o/dyld.h>

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

constexpr static const auto alignment = sizeof(void*);

static constexpr inline auto align(uintptr_t ptr, bool up = true) -> uintptr_t {
    if (ptr % alignment != 0) {
        if (up) {
            ptr = ptr + alignment - ptr % alignment;
        } else {
            ptr = ptr - alignment + ptr % alignment;
        }
    }
    return ptr;
}

static inline auto align(const void* ptr, bool up = true) -> uintptr_t {
    return align(reinterpret_cast<uintptr_t>(ptr), up);
}

void LSan::classifyRecord(MallocInfo& info, const LeakType& currentType) {
    auto stack = std::stack<std::reference_wrapper<MallocInfo>>();
    stack.push(info);
    while (!stack.empty()) {
        auto& elem = stack.top();
        stack.pop();
        if (elem.get().getLeakType() > currentType) {
            elem.get().setLeakType(currentType);
        }
        
        const auto beginPtr = align(info.getPointer());
        const auto   endPtr = align(beginPtr + info.getSize(), false);
        
        for (uintptr_t it = beginPtr; it < endPtr; it += sizeof(uintptr_t)) {
            if (it < lowest || it > highest) continue;
            const auto& record = infos.find(*reinterpret_cast<void**>(it));
            if (record == infos.end()
                || record->second.isDeleted()
                || record->second.getPointer() == info.getPointer()
                || record->second.getPointer() == elem.get().getPointer()
                || record->second.getLeakType() <= currentType) {
                continue;
            }
            stack.push(record->second);
        }
    }
}

#if !LSAN_CAN_WALK_STACK
LSAN_DIAGNOSTIC_PUSH
LSAN_IGNORE_FRAME_ADDRESS
static inline auto getFrameAddress(unsigned level) -> void* {
    switch (level) {
        case   0: return __builtin_frame_address  (0);
        case   1: return __builtin_frame_address  (1);
        case   2: return __builtin_frame_address  (2);
        case   3: return __builtin_frame_address  (3);
        case   4: return __builtin_frame_address  (4);
        case   5: return __builtin_frame_address  (5);
        case   6: return __builtin_frame_address  (6);
        case   7: return __builtin_frame_address  (7);
        case   8: return __builtin_frame_address  (8);
        case   9: return __builtin_frame_address  (9);
        case  10: return __builtin_frame_address (10);
        case  11: return __builtin_frame_address (11);
        case  12: return __builtin_frame_address (12);
        case  13: return __builtin_frame_address (13);
        case  14: return __builtin_frame_address (14);
        case  15: return __builtin_frame_address (15);
        case  16: return __builtin_frame_address (16);
        case  17: return __builtin_frame_address (17);
        case  18: return __builtin_frame_address (18);
        case  19: return __builtin_frame_address (19);
        case  20: return __builtin_frame_address (20);
        case  21: return __builtin_frame_address (21);
        case  22: return __builtin_frame_address (22);
        case  23: return __builtin_frame_address (23);
        case  24: return __builtin_frame_address (24);
        case  25: return __builtin_frame_address (25);
        case  26: return __builtin_frame_address (26);
        case  27: return __builtin_frame_address (27);
        case  28: return __builtin_frame_address (28);
        case  29: return __builtin_frame_address (29);
        case  30: return __builtin_frame_address (30);
        case  31: return __builtin_frame_address (31);
        case  32: return __builtin_frame_address (32);
        case  33: return __builtin_frame_address (33);
        case  34: return __builtin_frame_address (34);
        case  35: return __builtin_frame_address (35);
        case  36: return __builtin_frame_address (36);
        case  37: return __builtin_frame_address (37);
        case  38: return __builtin_frame_address (38);
        case  39: return __builtin_frame_address (39);
        case  40: return __builtin_frame_address (40);
        case  41: return __builtin_frame_address (41);
        case  42: return __builtin_frame_address (42);
        case  43: return __builtin_frame_address (43);
        case  44: return __builtin_frame_address (44);
        case  45: return __builtin_frame_address (45);
        case  46: return __builtin_frame_address (46);
        case  47: return __builtin_frame_address (47);
        case  48: return __builtin_frame_address (48);
        case  49: return __builtin_frame_address (49);
        case  50: return __builtin_frame_address (50);
        case  51: return __builtin_frame_address (51);
        case  52: return __builtin_frame_address (52);
        case  53: return __builtin_frame_address (53);
        case  54: return __builtin_frame_address (54);
        case  55: return __builtin_frame_address (55);
        case  56: return __builtin_frame_address (56);
        case  57: return __builtin_frame_address (57);
        case  58: return __builtin_frame_address (58);
        case  59: return __builtin_frame_address (59);
        case  60: return __builtin_frame_address (60);
        case  61: return __builtin_frame_address (61);
        case  62: return __builtin_frame_address (62);
        case  63: return __builtin_frame_address (63);
        case  64: return __builtin_frame_address (64);
        case  65: return __builtin_frame_address (65);
        case  66: return __builtin_frame_address (66);
        case  67: return __builtin_frame_address (67);
        case  68: return __builtin_frame_address (68);
        case  69: return __builtin_frame_address (69);
        case  70: return __builtin_frame_address (70);
        case  71: return __builtin_frame_address (71);
        case  72: return __builtin_frame_address (72);
        case  73: return __builtin_frame_address (73);
        case  74: return __builtin_frame_address (74);
        case  75: return __builtin_frame_address (75);
        case  76: return __builtin_frame_address (76);
        case  77: return __builtin_frame_address (77);
        case  78: return __builtin_frame_address (78);
        case  79: return __builtin_frame_address (79);
        case  80: return __builtin_frame_address (80);
        case  81: return __builtin_frame_address (81);
        case  82: return __builtin_frame_address (82);
        case  83: return __builtin_frame_address (83);
        case  84: return __builtin_frame_address (84);
        case  85: return __builtin_frame_address (85);
        case  86: return __builtin_frame_address (86);
        case  87: return __builtin_frame_address (87);
        case  88: return __builtin_frame_address (88);
        case  89: return __builtin_frame_address (89);
        case  90: return __builtin_frame_address (90);
        case  91: return __builtin_frame_address (91);
        case  92: return __builtin_frame_address (92);
        case  93: return __builtin_frame_address (93);
        case  94: return __builtin_frame_address (94);
        case  95: return __builtin_frame_address (95);
        case  96: return __builtin_frame_address (96);
        case  97: return __builtin_frame_address (97);
        case  98: return __builtin_frame_address (98);
        case  99: return __builtin_frame_address (99);
        case 100: return __builtin_frame_address(100);
        case 101: return __builtin_frame_address(101);
        case 102: return __builtin_frame_address(102);
        case 103: return __builtin_frame_address(103);
        case 104: return __builtin_frame_address(104);
        case 105: return __builtin_frame_address(105);
        case 106: return __builtin_frame_address(106);
        case 107: return __builtin_frame_address(107);
        case 108: return __builtin_frame_address(108);
        case 109: return __builtin_frame_address(109);
        case 110: return __builtin_frame_address(110);
        case 111: return __builtin_frame_address(111);
        case 112: return __builtin_frame_address(112);
        case 113: return __builtin_frame_address(113);
        case 114: return __builtin_frame_address(114);
        case 115: return __builtin_frame_address(115);
        case 116: return __builtin_frame_address(116);
        case 117: return __builtin_frame_address(117);
        case 118: return __builtin_frame_address(118);
        case 119: return __builtin_frame_address(119);
        case 120: return __builtin_frame_address(120);
        case 121: return __builtin_frame_address(121);
        case 122: return __builtin_frame_address(122);
        case 123: return __builtin_frame_address(123);
        case 124: return __builtin_frame_address(124);
        case 125: return __builtin_frame_address(125);
        case 126: return __builtin_frame_address(126);
        case 127: return __builtin_frame_address(127);
    }
    return nullptr;
}
LSAN_DIAGNOSTIC_POP
#endif

static inline auto findStackBegin() -> void* {
    void* here = __builtin_frame_address(0);
    
    void* toReturn;
    
#ifdef LSAN_STACK_X86
    void** frameBasePointer = reinterpret_cast<void**>(here);
    void** previousFBP = nullptr;
    while (frameBasePointer > previousFBP) {
        previousFBP = frameBasePointer;
        frameBasePointer = reinterpret_cast<void**>(frameBasePointer[0]);
    }
    toReturn = previousFBP;
#else // TODO: Implement properly for ARM (64)
    toReturn = nullptr;
    void* it;
    for (unsigned i = 0; i < 128 && (it = getFrameAddress(i)) != nullptr; ++i) {
        toReturn = it;
    }
#endif
    
    return toReturn;
}

struct Region {
    void *begin, *end;
    
    Region(void* begin, void* end): begin(begin), end(end) {}
};

static inline auto getAvailableRegions() -> std::vector<Region> {
    auto toReturn = std::vector<Region>();
    
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const mach_header* header = _dyld_get_image_header(i);
        if (header->magic != MH_MAGIC_64) continue;
        
        load_command* lc = reinterpret_cast<load_command*>(reinterpret_cast<uintptr_t>(header) + sizeof(mach_header_64));
        for (uint32_t j = 0; j < header->ncmds; ++j) {
            switch (lc->cmd) {
                case LC_SEGMENT_64: {
                    auto seg = reinterpret_cast<segment_command_64*>(lc);
                    uintptr_t ptr = _dyld_get_image_vmaddr_slide(i) + seg->vmaddr;
                    auto end = ptr + seg->vmsize;
                    // TODO: Filter out bss
                    if (seg->initprot & 2 && seg->initprot & 1) // 2: Read 1: Write
                        toReturn.push_back(Region(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(end)));
                    break;
                }
            }
            lc = reinterpret_cast<load_command*>(reinterpret_cast<uintptr_t>(lc) + lc->cmdsize);
        }
    }
    
    return toReturn;
}

void LSan::classifyLeaks() {
    // TODO: Search on the other thread stacks
    // TODO: Ignore allocated TLVs of our thread - only care about their value
    
    // Search on our stack
    const auto  here = align(__builtin_frame_address(0), false);
    const auto begin = align(findStackBegin());
    classifyLeaks(here, begin, LeakType::reachableDirect, LeakType::reachableIndirect, true);
    
    // Search in global space
    const auto& regions = getAvailableRegions();
    for (const auto& region : regions) {
        classifyLeaks(align(region.begin), align(region.end, false), LeakType::globalDirect, LeakType::globalIndirect, true);
    }
    
    // All leaks still unclassified are unreachable, search for reachability inside them
    for (auto& [pointer, record] : infos) {
        if (record.getLeakType() != LeakType::unclassified || record.isDeleted()) {
            continue;
        }
        record.setLeakType(LeakType::unreachableDirect);
        classifyRecord(record, LeakType::unreachableIndirect);
    }
}

LSan::LSan(): libName(lsanName().value()) {
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
}

auto LSan::removeMalloc(void* pointer) -> MallocInfoRemoved {
    std::lock_guard lock(infoMutex);
    
    auto it = infos.find(pointer);
    if (it == infos.end()) {
        return MallocInfoRemoved(false, std::nullopt);
    } else if (it->second.isDeleted()) {
        return MallocInfoRemoved(false, it->second);
    }
    if (__lsan_statsActive) {
        stats -= it->second;
        it->second.setDeleted(true);
    } else {
        infos.erase(it);
    }
    return MallocInfoRemoved(true, std::nullopt);
}

auto LSan::changeMalloc(const MallocInfo& info) -> bool {
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
    maybeSetHighestOrLowest(info.getPointer());
    infos.insert_or_assign(info.getPointer(), info);
    return true;
}

void LSan::addMalloc(MallocInfo&& info) {
    std::lock_guard lock(infoMutex);
    
    if (__lsan_statsActive) {
        stats += info;
    }
    
    maybeSetHighestOrLowest(info.getPointer());
    infos.insert_or_assign(info.getPointer(), info);
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
    
    self.classifyLeaks();
    
    // classify the leaks
    // create summary on the way
    // print summary
    // print lost memory - according to what types should be shown and with a note what kind of leak it is
    // print summary again
    // print hint for how to make the rest visible
    
    std::size_t i     = 0,
                j     = 0,
                bytes = 0,
                count = 0,
                total = self.infos.size();
    for (auto & [ptr, info] : self.infos) {
        assert(info.getLeakType() != LeakType::unclassified);
        if (isATTY()) {
            char buffer[7] {};
            std::snprintf(buffer, 7, "%05.2f", static_cast<double>(j) / total * 100);
            stream << "\rCollecting the leaks: " << formatter::format<Style::BOLD>(buffer) << " %";
        }
        if (!info.isDeleted() && callstackHelper::getCallstackType(info.getCreatedCallstack()) == callstackHelper::CallstackType::USER) {
            ++count;
            bytes += info.getSize();
            if (i < __lsan_leakCount) {
                if (isATTY()) {
                    stream << "\r";
                }
                stream << "Classified: " << info.getLeakType() << "                        " << std::endl;
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
