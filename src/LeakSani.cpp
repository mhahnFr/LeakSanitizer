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

#if defined(__x86_64__) || defined(__i386__)
 #define LSAN_CAN_WALK_STACK 1
 #define LSAN_STACK_X86
#elif defined(__arm64__) || defined(__arm__)
 #define LSAN_CAN_WALK_STACK 1
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
 #define LSAN_IGNORE_FRAME_ADDRESS
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
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
    /*
     TODO: Implement a way of getting the stack bounds without walking framepointers
     TODO: Consolidate the ARM and the x86 stackwalking implementations
     */

    void* toReturn;
    
#ifdef LSAN_STACK_X86
    void* here = __builtin_frame_address(0);
    
    void** frameBasePointer = reinterpret_cast<void**>(here);
    void** previousFBP = nullptr;
    while (frameBasePointer > previousFBP) {
        previousFBP = frameBasePointer;
        frameBasePointer = reinterpret_cast<void**>(frameBasePointer[0]);
    }
    toReturn = previousFBP;
#elif defined(LSAN_STACK_ARM)
    void* previousFrame = nullptr;
    void* frame = __builtin_frame_address(0);

    while (frame > previousFrame) {
        previousFrame = frame;
        frame = *reinterpret_cast<void**>(frame);
    }
    toReturn = previousFrame;
#else
    toReturn = nullptr;
    void* it;
    for (unsigned i = 0; i < 128 && (it = getFrameAddress(i)) != nullptr; ++i) {
        toReturn = it;
    }
#endif
    
    return toReturn;
}

static inline auto getGlobalRegionsAndTLVs() -> std::pair<std::vector<Region>, std::set<const void*>> {
    auto regions = std::vector<Region>();
    auto locals  = std::set<const void*>();

#ifdef __APPLE__
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const mach_header* header = _dyld_get_image_header(i);
        if (header->magic != MH_MAGIC_64) continue;
        
        load_command* lc = reinterpret_cast<load_command*>(reinterpret_cast<uintptr_t>(header) + sizeof(mach_header_64));
        for (uint32_t j = 0; j < header->ncmds; ++j) {
            switch (lc->cmd) {
                case LC_SEGMENT_64: {
                    auto seg = reinterpret_cast<segment_command_64*>(lc);
                    const auto vmaddrslide = _dyld_get_image_vmaddr_slide(i);
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
                                    locals.insert(desc->thunk(desc));
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
    return std::make_pair(regions, locals);
}

void LSan::classifyStackLeaksShallow() {
    std::lock_guard lock(infoMutex);

    // TODO: Care about the stack direction
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

void LSan::addTLSKey(const pthread_key_t& key) {
    std::lock_guard lock { tlsKeyMutex };

    keys.insert(key);
}

auto LSan::removeTLSKey(const pthread_key_t& key) -> bool {
    std::lock_guard lock { tlsKeyMutex };

    return keys.erase(key) != 0;
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
