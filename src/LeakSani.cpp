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
#include <stack>

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

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#endif

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

auto LSan::classifyRecord(MallocInfo& info, const LeakType& currentType) -> std::pair<std::size_t, std::size_t> {
    std::size_t count { 0 },
                bytes { 0 };
    auto stack = std::stack<std::reference_wrapper<MallocInfo>>();
    stack.push(info);
    while (!stack.empty()) {
        auto& elem = stack.top();
        stack.pop();
        if (elem.get().leakType > currentType && elem.get().pointer != info.pointer) {
            elem.get().leakType = currentType;
            ++count;
            bytes += elem.get().size;
        }
        
        const auto beginPtr = align(elem.get().pointer);
        const auto   endPtr = align(beginPtr + elem.get().size, false);

        for (uintptr_t it = beginPtr; it < endPtr; it += sizeof(uintptr_t)) {
            const auto& record = infos.find(*reinterpret_cast<void**>(it));
            if (record == infos.end()
                || record->second.deleted
                || record->second.pointer == info.pointer
                || record->second.pointer == elem.get().pointer) {
                continue;
            }
            info.viaMeRecords.insert(&record->second);
            if (record->second.leakType > currentType)
                stack.push(record->second);
        }
    }
    return std::make_pair(count, bytes);
}

static inline auto findStackBegin(pthread_t thread = pthread_self()) -> void* {
    void* toReturn;

    // TODO: Linux version
#ifdef __APPLE__
    toReturn = pthread_get_stackaddr_np(thread);
#endif

    return toReturn;
}

#ifdef __APPLE__
static inline void getGlobalRegionsAndTLVs(const mach_header* header, intptr_t vmaddrslide, std::vector<Region>& regions, std::set<const void*>& tlvs) {
    if (header->magic != MH_MAGIC_64) return;

    load_command* lc = reinterpret_cast<load_command*>(reinterpret_cast<uintptr_t>(header) + sizeof(mach_header_64));
    for (uint32_t j = 0; j < header->ncmds; ++j) {
        switch (lc->cmd) {
            case LC_SEGMENT_64: {
                auto seg = reinterpret_cast<segment_command_64*>(lc);
                if (strcmp(seg->segname, SEG_TEXT) == 0) {
                    if (vmaddrslide == 0) vmaddrslide = (uintptr_t)header - seg->vmaddr;
                }
                uintptr_t ptr = vmaddrslide + seg->vmaddr;
                auto end = ptr + seg->vmsize;
                // TODO: Filter out bss
                if (seg->initprot & 2 && seg->initprot & 1) // 2: Read 1: Write
                {
                    regions.push_back(Region(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(end)));

                    auto sect = reinterpret_cast<section_64*>(reinterpret_cast<uintptr_t>(seg) + sizeof(*seg));
                    for (uint32_t i = 0; i < seg->nsects; ++i) {
                        if (sect->flags == S_THREAD_LOCAL_VARIABLES) {
                            struct tlv_descriptor* desc = reinterpret_cast<tlv_descriptor*>(vmaddrslide + sect->addr);

                            uintptr_t de = reinterpret_cast<uintptr_t>(desc) + sect->size;
                            for (tlv_descriptor* d = desc; reinterpret_cast<uintptr_t>(d) < de; ++d) {
                                tlvs.insert(d->thunk(d));
                            }
                        }
                        sect = reinterpret_cast<section_64*>(reinterpret_cast<uintptr_t>(sect) + sizeof(section_64));
                    }
                }

                break;
            }
        }
        lc = reinterpret_cast<load_command*>(reinterpret_cast<uintptr_t>(lc) + lc->cmdsize);
    }
}
#endif

static inline auto getGlobalRegionsAndTLVs() -> std::pair<std::vector<Region>, std::set<const void*>> {
    auto regions = std::vector<Region>();
    auto locals  = std::set<const void*>();

#ifdef __APPLE__
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        getGlobalRegionsAndTLVs(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i), regions, locals);
    }

    struct task_dyld_info dyldInfo;
    mach_msg_type_number_t infoCount = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t) &dyldInfo, &infoCount) == KERN_SUCCESS) {
        struct dyld_all_image_infos* infos = (struct dyld_all_image_infos*) dyldInfo.all_image_info_addr;
        getGlobalRegionsAndTLVs(infos->dyldImageLoadAddress, 0, regions, locals);
    } else {
        // TODO: Handle the error
    }
#endif
    return std::make_pair(regions, locals);
}

void LSan::classifyStackLeaksShallow() {
    std::lock_guard lock(infoMutex);

    // TODO: Care about the stack direction
    // TODO: Care about the stacks of leaked threads

    const auto  here = align(__builtin_frame_address(0), false);
    const auto begin = align(findStackBegin());
    for (uintptr_t it = here; it < begin; it += sizeof(uintptr_t)) {
        const auto& record = infos.find(*reinterpret_cast<void**>(it));
        if (record != infos.end() && !record->second.deleted) {
            record->second.leakType = LeakType::reachableDirect;
        }
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

auto LSan::classifyLeaks() -> LeakKindStats {
    // TODO: Search on the other thread stacks
    auto toReturn = LeakKindStats();

    auto& out = getOutputStream();
    const auto& clear = [](std::ostream& out) -> std::ostream& {
        if (isATTY()) {
            return out << "\r                                                        \r";
        }
        return out << std::endl;
    };
    out << "Searching globals and compile time thread locals...";
    const auto& [regions, locals] = getGlobalRegionsAndTLVs();
    out << clear << "Collecting the leaks...";
    for (auto it = infos.begin(); it != infos.end();) {
        const auto& local = locals.find(it->first);
        if (local != locals.end() || it->second.deleted || callstackHelper::getCallstackType(it->second.createdCallstack) != callstackHelper::CallstackType::USER) {
            it = infos.erase(it);
        } else {
            ++it;
        }
    }

    out << clear << "Reachability analysis: Stack, part I...";
    for (auto& [ptr, record] : infos) {
        if (record.leakType == LeakType::reachableDirect) {
            ++toReturn.stack;
            toReturn.bytesStack += record.size;
            const auto& [count, bytes] = classifyRecord(record, LeakType::reachableIndirect);
            toReturn.stackIndirect += count;
            toReturn.bytesStackIndirect += bytes;
            toReturn.recordsStack.insert(&record);
        }
    }

    out << clear << "Reachability analysis: Stack, part II...";
    // Search on our stack
    const auto  here = align(__builtin_frame_address(0), false);
    const auto begin = align(findStackBegin());
    const auto& [stackHereDirect, stackHereBytes, 
                 stackHereIndirect, stackHereBytesIndirect] = classifyLeaks(here, begin,
                                                                            LeakType::reachableDirect, LeakType::reachableIndirect,
                                                                            toReturn.recordsStack, true);
    toReturn.stack += stackHereDirect;
    toReturn.stackIndirect += stackHereIndirect;
    toReturn.bytesStack += stackHereBytes;
    toReturn.bytesStackIndirect += stackHereBytesIndirect;

    out << clear << "Reachability analysis: Globals...";
    // Search in global space
    for (const auto& region : regions) {
        const auto& [regionDirect, regionBytes,
                     regionIndirect, regionBytesIndirect] = classifyLeaks(align(region.begin), align(region.end, false),
                                                                          LeakType::globalDirect, LeakType::globalIndirect,
                                                                          toReturn.recordsGlobal, true);
        toReturn.global += regionDirect;
        toReturn.globalIndirect += regionIndirect;
        toReturn.bytesGlobal += regionBytes;
        toReturn.bytesGlobalIndirect += regionBytesIndirect;
    }
    
    out << clear << "Reachability analysis: Runtime thread-local variables...";
    // Search in the runtime thread locals
    for (const auto& key : keys) {
        const auto value = pthread_getspecific(key);
        if (value == nullptr) continue;

        const auto& it = infos.find(value);
        if (it == infos.end()) continue;

        it->second.leakType = LeakType::tlvDirect;
        const auto& [count, bytes] = classifyRecord(it->second, LeakType::tlvIndirect);
        toReturn.tlvIndirect += count;
        toReturn.bytesTlvIndirect += bytes;
        ++toReturn.tlv;
        toReturn.bytesTlv += it->second.size;
        toReturn.recordsTlv.insert(&it->second);
    }

    out << clear << "Reachability analysis: Lost memory...";
    // All leaks still unclassified are unreachable, search for reachability inside them
    for (auto& [pointer, record] : infos) {
        if (record.leakType != LeakType::unclassified || record.deleted) {
            continue;
        }
        record.leakType = LeakType::unreachableDirect;
        const auto& [count, bytes] = classifyRecord(record, LeakType::unreachableIndirect);
        toReturn.lostIndirect += count;
        toReturn.bytesLostIndirect += bytes;
        toReturn.recordsLost.insert(&record);
    }
    out << clear << "Enumerating lost memory leaks...";
    toReturn.lost = infos.size()
                    - toReturn.stack - toReturn.stackIndirect
                    - toReturn.global - toReturn.globalIndirect
                    - toReturn.lostIndirect
                    - toReturn.tlv - toReturn.tlvIndirect;
    for (const auto& record : toReturn.recordsLost) {
        if (record->leakType != LeakType::unreachableDirect) continue;

        toReturn.bytesLost += record->size;
    }
    out << clear << clear;
    return toReturn;
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

auto LSan::addTLSValue(const pthread_key_t& key, const void* value) -> bool {
    if (!hasTLSKey(key)) {
        return false;
    }

    std::lock_guard lock { tlsKeyValuesMutex };
    
    tlsKeyValues.insert_or_assign(std::make_pair(pthread_self(), key), value);

    return true;
}

void LSan::addTLSKey(const pthread_key_t& key) {
    std::lock_guard lock { tlsKeyMutex };

    keys.insert(key);
}

auto LSan::removeTLSKey(const pthread_key_t& key) -> bool {
    std::lock_guard lock { tlsKeyMutex };

    return keys.erase(key) != 0;
}

auto LSan::hasTLSKey(const pthread_key_t& key) -> bool {
    // TODO: Collect all TLS keys - also the compile time ones
    std::lock_guard lock { tlsKeyMutex };

    return keys.count(key) != 0;
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
    
    const auto& stats = self.classifyLeaks();

    // [x] classify the leaks
    // [x] create summary on the way
    // [x] print summary
    // [x] print lost memory - according to what types should be shown and with a note what kind of leak it is
    // [x] print summary again
    // [x] print hint for how to make the rest visible

    if (stats.getTotal() > 0) {
        // TODO: Optionally collaps identical callstacks
        // TODO: Return early if no leaks have been detected
        // TODO: Further formatting
        // TODO: Maybe split between direct and indirect?
        stream << "Total: " << stats.getTotal() << " leaks (" << bytesToString(stats.getTotalBytes()) << ")" << std::endl
               << "       " << stats.getTotalLost() << " leaks (" << bytesToString(stats.getLostBytes()) << ") lost" << std::endl
               << "       " << stats.getTotalReachable() << " leaks (" << bytesToString(stats.getReachableBytes()) << ") reachable" << std::endl
               << std::endl;

        for (const auto& record : stats.recordsLost) {
            if (record->leakType != LeakType::unreachableDirect) continue;

            stream << *record << std::endl;
        }

        // TODO: Possibility to show indirects

        if ((true)) { // TODO: If should show reachables
            for (const auto& record : stats.recordsGlobal) {
                stream << *record << std::endl;
            }
            for (const auto& record : stats.recordsTlv) {
                stream << *record << std::endl;
            }
            for (const auto& record : stats.recordsStack) {
                stream << *record << std::endl;
            }
        } else {
            stream << "Set LSAN_SHOW_REACHABLES to true to display reachable memory leaks." << std::endl;
        }

        if (self.callstackSizeExceeded) {
            stream << printCallstackSizeExceeded;
            self.callstackSizeExceeded = false;
        }
        if (__lsan_relativePaths) {
            stream << printWorkingDirectory;
        }
    } else {
        stream << "No leaks detected." << std::endl;
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
    if (stats.getTotal() > 0) {
        // TODO: Further formatting
        stream << std::endl << "Summary:" << std::endl
               << "Total: " << stats.getTotal() << " leaks (" << bytesToString(stats.getTotalBytes()) << ")" << std::endl
               << "       " << stats.getTotalLost() << " leaks (" << bytesToString(stats.getLostBytes()) << ") lost" << std::endl
               << "       " << stats.getTotalReachable() << " leaks (" << bytesToString(stats.getReachableBytes()) << ") reachable" << std::endl;
    }

    callstack_clearCaches();
    callstack_autoClearCaches = true;
    
#ifdef BENCHMARK
    stream << std::endl << timing::printTimings << std::endl;
#endif

    return stream;
}
}
