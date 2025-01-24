/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2025  mhahnFr
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
#include <filesystem>
#include <stack>

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

#include <objc/runtime.h>
#include <CoreFoundation/CFDictionary.h>

#define OBJC_SUPPORT_EXTRA 1
#include "objcSupport.hpp"
#endif

namespace lsan {
std::atomic_bool LSan::finished = false;
std::atomic_bool LSan::preventDealloc = false;

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

auto LSan::findWithSpecials(void* ptr) -> decltype(infos)::iterator {
    auto toReturn = infos.find(ptr);
    if (toReturn == infos.end()) {
        toReturn = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) - 2 * sizeof(void*)));
    }
    if (toReturn == infos.end()) {
        toReturn = infos.find(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(void*)));
    }
    return toReturn;
}

void LSan::classifyLeaks(uintptr_t begin, uintptr_t end,
                         LeakType direct, LeakType indirect,
                         std::deque<MallocInfo::Ref>& directs, bool skipClassifieds,
                         const char* name, const char* nameRelative) {
    for (uintptr_t it = begin; it < end; it += sizeof(uintptr_t)) {
        const auto& record = infos.find(*reinterpret_cast<void**>(it));
        if (record == infos.end() || record->second.deleted || (skipClassifieds && record->second.leakType != LeakType::unclassified)) {
            continue;
        }
        if (record->second.leakType > direct) {
            record->second.leakType = direct;
            record->second.imageName.first = name;
            record->second.imageName.second = nameRelative;
            directs.push_back(record->second);
        }
        classifyRecord(record->second, indirect);
    }
}

void LSan::classifyClass(void* cls, std::deque<MallocInfo::Ref>& directs, LeakType direct, LeakType indirect) {
    auto classWords = reinterpret_cast<void**>(cls);
    auto cachePtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[2]) & (((uintptr_t) 1 << 48) - 1));
    const auto& cacheIt = infos.find(cachePtr);
    if (cacheIt != infos.end() && cacheIt->second.leakType > direct) {
        cacheIt->second.leakType = direct;
        classifyRecord(cacheIt->second, indirect);
        directs.push_back(cacheIt->second);
    }

    auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(classWords[4]) & 0x0f007ffffffffff8UL);
    const auto& it = infos.find(ptr);
    if (it != infos.end()) {
        if (it->second.leakType > direct) {
            it->second.leakType = direct;
            classifyRecord(it->second, indirect);
            directs.push_back(it->second);
        }

        auto rwStuff = reinterpret_cast<void**>(it->second.pointer);
        auto ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rwStuff[1]) & ~1);
        const auto& it = infos.find(ptr);
        if (it != infos.end()) {
            if (it->second.leakType > direct) {
                it->second.leakType = direct;
                classifyRecord(it->second, indirect);
                directs.push_back(it->second);
            }
            if (it->second.size >= 4 * sizeof(void*)) {
                const auto ptrArr = reinterpret_cast<void**>(it->second.pointer);
                for (unsigned char i = 1; i < 4; ++i) {
                    classifyPointerUnion<true>(ptrArr[i], directs, direct, indirect);
                }
            }
        }
    }
}

void LSan::classifyRecord(MallocInfo& info, const LeakType& currentType) {
    auto stack = std::stack<std::reference_wrapper<MallocInfo>>();
    stack.push(info);
    while (!stack.empty()) {
        auto& elem = stack.top();
        stack.pop();
        if (elem.get().leakType > currentType && elem.get().pointer != info.pointer) {
            elem.get().leakType = currentType;
        }

        const auto beginPtr = align(elem.get().pointer);
        const auto   endPtr = align(beginPtr + elem.get().size, false);

        for (uintptr_t it = beginPtr; it < endPtr; it += sizeof(uintptr_t)) {
            const auto& record = findWithSpecials(*reinterpret_cast<void**>(it));
            if (record == infos.end()
                || record->second.deleted
                || record->second.pointer == info.pointer
                || record->second.pointer == elem.get().pointer) {
                continue;
            }
            info.viaMeRecords.push_back(record->second);
            if (record->second.leakType > currentType)
                stack.push(record->second);
        }
    }
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
static inline void getGlobalRegionsAndTLVs(const mach_header* header, intptr_t vmaddrslide,
                                           std::vector<Region>& regions,
                                           std::vector<std::tuple<const void*, const char*, const char*>>& tlvs,
                                           const char* name, const char* relative) {
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
                    regions.push_back(Region { reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(end), name, relative });

                    auto sect = reinterpret_cast<section_64*>(reinterpret_cast<uintptr_t>(seg) + sizeof(*seg));
                    for (uint32_t i = 0; i < seg->nsects; ++i) {
                        if (sect->flags == S_THREAD_LOCAL_VARIABLES) {
                            struct tlv_descriptor* desc = reinterpret_cast<tlv_descriptor*>(vmaddrslide + sect->addr);

                            uintptr_t de = reinterpret_cast<uintptr_t>(desc) + sect->size;
                            for (tlv_descriptor* d = desc; reinterpret_cast<uintptr_t>(d) < de; ++d) {
                                tlvs.push_back(std::make_tuple(d->thunk(d), name, relative));
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

auto LSan::getGlobalRegionsAndTLVs(std::vector<std::pair<char*, char*>>& binaryFilenames) -> std::pair<std::vector<Region>, std::vector<std::tuple<const void*, const char*, const char*>>> {
    auto regions = std::vector<Region>();
    auto locals  = std::vector<std::tuple<const void*, const char*, const char*>>();

#ifdef __APPLE__
    const uint32_t count = _dyld_image_count();
    for (uint32_t i = 0; i < count; ++i) {
        const auto& filename = strdup(_dyld_get_image_name(i));
        const auto& relative = getBehaviour().relativePaths() ? strdup(std::filesystem::relative(filename).string().c_str()) : nullptr;
        binaryFilenames.push_back({ filename, relative });
        ::lsan::getGlobalRegionsAndTLVs(_dyld_get_image_header(i), _dyld_get_image_vmaddr_slide(i), regions, locals, filename, relative);
    }

    struct task_dyld_info dyldInfo;
    mach_msg_type_number_t infoCount = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t) &dyldInfo, &infoCount) == KERN_SUCCESS) {
        struct dyld_all_image_infos* infos = (struct dyld_all_image_infos*) dyldInfo.all_image_info_addr;
        const auto& filename = strdup(infos->dyldPath);
        const auto& relative = getBehaviour().relativePaths() ? strdup(std::filesystem::relative(filename).string().c_str()) : nullptr;
        binaryFilenames.push_back({ filename, relative });
        dyldPath = filename;
        ::lsan::getGlobalRegionsAndTLVs(infos->dyldImageLoadAddress, 0, regions, locals, filename, relative);
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

auto LSan::isInDyld(const MallocInfo& info) const -> bool {
    return info.imageName.first == dyldPath;
}

auto LSan::isSuppressed(const MallocInfo& info) -> bool {
    for (const auto& suppression : getSuppressions()) {
        if (isInDyld(info) || suppression.match(info)) {
            return true;
        }
    }
    return false;
}

void LSan::applySuppressions(const std::deque<MallocInfo::Ref>& leaks) {
    for (const auto& leak : leaks) {
        if (isSuppressed(leak.get()) && !leak.get().suppressed) {
            leak.get().markSuppressed();
        }
    }
}

auto LSan::classifyLeaks() -> LeakKindStats {
    // TODO: Search on the other thread stacks
    auto toReturn = LeakKindStats();

    auto& out = getOutputStream();
    const auto& clear = [](std::ostream& out) -> std::ostream& {
        if (isATTY()) {
            return out << "\r                                                             \r";
        }
        return out << std::endl;
    };
    out << "Searching globals and compile time thread locals...";
    auto [regions, locals] = getGlobalRegionsAndTLVs(binaryFilenames);

    out << clear << "Collecting the leaks...";
    for (auto it = infos.begin(); it != infos.end();) {
        if (it->second.deleted) {
            it = infos.erase(it);
        } else {
            ++it;
        }
    }

    out << clear << "Reachability analysis: Stack, part I...";
    for (auto& [ptr, record] : infos) {
        if (record.leakType == LeakType::reachableDirect) {
            classifyRecord(record, LeakType::reachableIndirect);
            toReturn.recordsStack.push_back(record);
        }
    }

    out << clear << "Reachability analysis: Stack, part II...";
    // Search on our stack
    const auto  here = align(__builtin_frame_address(0), false);
    const auto begin = align(findStackBegin());
    classifyLeaks(here, begin,
                  LeakType::reachableDirect, LeakType::reachableIndirect,
                  toReturn.recordsStack, true);

#ifdef LSAN_HANDLE_OBJC
    out << clear << "Reachability analysis: Objective-C runtime...";
    // Search in the Objective-C runtime
    const auto& classNumber = objc_getClassList(nullptr, 0);
    auto classes = new Class[classNumber];
    objc_getClassList(classes, classNumber);
    for (int i = 0; i < classNumber; ++i) {
        classifyClass(classes[i], toReturn.recordsObjC, LeakType::objcDirect, LeakType::objcIndirect);

        const auto& meta = object_getClass(reinterpret_cast<id>(classes[i]));
        classifyClass(meta, toReturn.recordsObjC, LeakType::objcDirect, LeakType::objcIndirect);
    }
    delete[] classes;
#endif

    out << clear << "Reachability analysis: Compile-time thread-local variables...";
    // Search in compile-time thread locals - their wrapper will be suppressed
    for (const auto& [ptr, name, relative] : locals) {
        const auto& it = infos.find(ptr);
        if (it == infos.end()) continue;

        classifyLeaks(align(it->second.pointer), align(reinterpret_cast<uintptr_t>(it->second.pointer) + it->second.size, false),
                      LeakType::tlvDirect, LeakType::tlvIndirect, toReturn.recordsTlv, name, relative);
        it->second.suppressed = true;
    }

    out << clear << "Reachability analysis: Runtime thread-local variables...";
    // Search in the runtime thread locals
    for (const auto& key : keys) {
        const auto value = pthread_getspecific(key);
        if (value == nullptr) continue;

        const auto& it = infos.find(value);
        if (it == infos.end()) continue;

        it->second.leakType = LeakType::tlvDirect;
        classifyRecord(it->second, LeakType::tlvIndirect);
        toReturn.recordsTlv.push_back(it->second);
    }

#ifdef LSAN_HANDLE_OBJC
    out << clear << "Reachability analysis: Cocoa thread-local variables...";
    // Search in the Cocoa runtime thread-locals
    // TODO: Get dicts of all threads
    const auto& dict = reinterpret_cast<CFDictionaryRef>(_2(_4(NSThread, currentThread), threadDictionary));
    const auto& count = CFDictionaryGetCount(dict);
    auto keys = new const void*[count];
    auto values = new const void*[count];
    CFDictionaryGetKeysAndValues(dict, keys, values);
    for (CFIndex i = 0; i < count; ++i) {
        const auto& keyIt = infos.find(keys[i]);
        if (keyIt != infos.end()) {
            classifyRecord(keyIt->second, LeakType::tlvIndirect);
            keyIt->second.leakType = LeakType::tlvDirect;
            // TODO: Add thread id / name?
        }
        const auto& valIt = infos.find(values[i]);
        if (valIt != infos.end()) {
            classifyRecord(valIt->second, LeakType::tlvIndirect);
            keyIt->second.leakType = LeakType::tlvDirect;
            // TODO: Add thread id / name?
        }
    }
    delete[] keys;
    delete[] values;

#endif

    out << clear << "Reachability analysis: Globals...";
    // Search in global space
    for (const auto& region : regions) {
        classifyLeaks(align(region.begin), align(region.end, false),
                      LeakType::globalDirect, LeakType::globalIndirect,
                      toReturn.recordsGlobal, true, region.name, region.nameRelative);
    }

    out << clear << "Reachability analysis: Lost memory...";
    // All leaks still unclassified are unreachable, search for reachability inside them
    for (auto& [pointer, record] : infos) {
        if (record.leakType != LeakType::unclassified || record.deleted) {
            continue;
        }
        record.leakType = LeakType::unreachableDirect;
        classifyRecord(record, LeakType::unreachableIndirect);
        toReturn.recordsLost.push_back(record);
    }

    out << clear << "Filtering the memory leaks...";
    for (const auto& leak : toReturn.recordsObjC) {
        if (!leak.get().suppressed) {
            leak.get().markSuppressed();
        }
    }
    applySuppressions(toReturn.recordsStack);
    applySuppressions(toReturn.recordsTlv);
    applySuppressions(toReturn.recordsGlobal);
    applySuppressions(toReturn.recordsLost);

    out << clear << "Enumerating memory leaks...";
#define ENUMERATE(records, count, bytes, indirect, indirectBytes) \
for (const auto& leak : (records)) {                              \
    if (leak.get().suppressed || leak.get().enumerated) continue; \
                                                                  \
    ++(count);                                                    \
    (bytes) += leak.get().size;                                   \
    const auto& [a, b] = leak.get().enumerate();                  \
    (indirect) += a;                                              \
    (indirectBytes) += b;                                         \
}
    ENUMERATE(toReturn.recordsStack, toReturn.stack, toReturn.bytesStack,
              toReturn.stackIndirect, toReturn.bytesStackIndirect)
    ENUMERATE(toReturn.recordsTlv, toReturn.tlv, toReturn.bytesTlv,
              toReturn.tlvIndirect, toReturn.bytesTlvIndirect)
    ENUMERATE(toReturn.recordsGlobal, toReturn.global, toReturn.bytesGlobal,
              toReturn.globalIndirect, toReturn.bytesGlobalIndirect)
    ENUMERATE(toReturn.recordsLost, toReturn.lost, toReturn.bytesLost,
              toReturn.lostIndirect, toReturn.bytesLostIndirect)
#undef ENUMERATE

    out << clear << clear;

    out << "   Total: " << toReturn.getTotal() << " (" << infos.size() << ")"<< std::endl
        << "    Lost: " << toReturn.lost << " (" << toReturn.recordsLost.size() << ")" << std::endl
        << "Indirect: " << toReturn.lostIndirect << std::endl
        << "  Global: " << toReturn.global << " (" << toReturn.recordsGlobal.size() << ")" << std::endl
        << "Indirect: " << toReturn.globalIndirect << std::endl
        << "     TLV: " << toReturn.tlv << " (" << toReturn.recordsTlv.size() << ")" << std::endl
        << "Indirect: " << toReturn.tlvIndirect << std::endl
        << "   Stack: " << toReturn.stack << " (" << toReturn.recordsStack.size() << ")" << std::endl
        << "Indirect: " << toReturn.stackIndirect << std::endl;

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
    if (behaviour.statsActive()) {
        stats -= it->second;
    }
    if (behaviour.statsActive()) {
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
    if (behaviour.statsActive()) {
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
           << formatter::clear<Style::GREYED> << getBehaviour().callstackSize()
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

void LSan::addThread(ThreadInfo&& info) {
    threads.insert_or_assign(info.getId(), std::move(info));
}

void LSan::removeThread(const std::thread::id& id) {
    threads.at(id).beginFrameAddress = std::nullopt;
}

auto LSan::getSuppressions() -> const std::vector<suppression::Suppression>& {
    if (!suppressions) {
        suppressions = loadSuppressions();
    }
    return *suppressions;
}

auto LSan::getThreadDescription(const std::thread::id& threadId) -> std::string {
    if (threadId == mainId) {
        return "main thread";
    }
    // TODO: Create thread number: 'thread # XX'
    return "<< WIP >>";
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
    using namespace formatter;

    if (has("LSAN_PRINT_STATS_ON_EXIT")) {
        printDeprecation(out, "LSAN_PRINT_STATS_ON_EXIT", "__lsan_printStatsOnExit",
                         "is no longer supported and " + formatString<Style::BOLD>("deprecated since version 1.7"));
    }
    if (has("LSAN_PRINT_LICENSE")) {
        printDeprecation(out, "LSAN_PRINT_LICENSE", "__lsan_printLicense",
                         "is no longer supported and " + formatString<Style::BOLD>("deprecated since version 1.8"));
    }
    if (has("LSAN_PRINT_WEBSITE")) {
        printDeprecation(out, "LSAN_PRINT_WEBSITE", "__lsan_printWebsite",
                         "is no longer supported and " + formatString<Style::BOLD>("deprecated since version 1.8"));
    }
    if (has("LSAN_FIRST_PARTY_THRESHOLD")) {
        printDeprecation(out, "LSAN_FIRST_PARTY_THRESHOLD", "__lsan_firstPartyThreshold",
                         "is no longer supported and " + formatString<Style::BOLD>("deprecated since version 1.11"));
    }
    if (has("LSAN_FIRST_PARTY_REGEX")) {
        printDeprecation(out, "LSAN_FIRST_PARTY_REGEX", "__lsan_firstPartyRegex",
                         "is no longer supported and " + formatString<Style::BOLD>("deprecated since version 1.11"));
    }
    return out;
}

static inline void printRecord(std::ostream& out, const MallocInfo& info) {
    auto ptr = reinterpret_cast<void**>(info.pointer);
    for (std::size_t i = 0; i < info.size / 8; ++i) {
        out << ptr[i] << ", ";
    }
    out << std::endl << info.pointer << " ";
}

static inline void printRecords(const std::deque<MallocInfo::Ref>& records, std::ostream& out, bool printContent = false) {
    for (const auto& leak : records) {
        auto& record = leak.get();
        if (!record.printedInRoot && !record.suppressed && !record.printedInRoot) {
            if (printContent) {
                printRecord(out, record);
            }
            out << record << std::endl;
            record.printedInRoot = true;
        }
    }
}

static inline auto operator<<(std::ostream& out, const LeakKindStats& stats) -> std::ostream& {
    using namespace formatter;

    // TODO: Further formatting
    // TODO: Maybe split between direct and indirect?
    out << "Total: " << stats.getTotal() << " leaks (" << bytesToString(stats.getTotalBytes()) << ")" << std::endl
        << "       " << stats.getTotalLost() << " leaks (" << bytesToString(stats.getLostBytes()) << ") lost" << std::endl
        << "       " << stats.getTotalReachable() << " leaks (" << bytesToString(stats.getReachableBytes()) << ") reachable";
    if (!getBehaviour().showReachables()) {
        out << format<Style::ITALIC>(" (not shown)");
    }
    return out << std::endl;
}

auto operator<<(std::ostream& stream, LSan& self) -> std::ostream& {
    using namespace formatter;

    std::lock_guard lock(self.infoMutex);

    callstack_rawNames = self.behaviour.suppressionDevelopersMode();
    callstack_autoClearCaches = false;

    const auto& stats = self.classifyLeaks();
    if (stats.getTotal() > 0) {
        // TODO: Optionally collapse identical callstacks
        stream << stats << std::endl;

        for (const auto& leak : stats.recordsLost) {
            auto& record = leak.get();
            if (record.leakType != LeakType::unreachableDirect || record.printedInRoot || record.suppressed) continue;

//            printRecord(stream, *record);
            stream << record << std::endl;
            record.printedInRoot = true;
        }

        if (self.behaviour.showReachables()) {
            printRecords(stats.recordsTlv, stream);
            printRecords(stats.recordsGlobal, stream);
            printRecords(stats.recordsStack, stream);
        } else if (stats.getTotalReachable() > 0) {
            stream << "Hint: Set " << format<Style::BOLD>("LSAN_SHOW_REACHABLES") << " to "
                   << format<Style::BOLD>("true") << " to display the reachable memory leaks."
                   << std::endl << std::endl;
        }

        if (self.callstackSizeExceeded) {
            stream << printCallstackSizeExceeded;
            self.callstackSizeExceeded = false;
        }
        if (self.behaviour.relativePaths()) {
            stream << printWorkingDirectory;
        }
    } else {
        stream << format<Style::BOLD, Style::GREEN, Style::ITALIC>("No leaks detected.") << std::endl;
    }

    stream << maybeShowDeprecationWarnings;
    if (stats.getTotal() > 0) {
        stream << std::endl << format<Style::BOLD>("Summary:") << std::endl << stats;
    }

    for (const auto& string : self.binaryFilenames) {
        std::free(string.first);
        std::free(string.second);
    }
    callstack_clearCaches();
    callstack_autoClearCaches = true;
    
#ifdef BENCHMARK
    stream << std::endl << timing::printTimings << std::endl;
#endif

    return stream;
}
}
