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
#include <functionInfo/functionInfo.h>
#include <regions/regions.h>

#include "LeakSani.hpp"

#include "bytePrinter.hpp"
#include "formatter.hpp"
#include "lsanMisc.hpp"
#include "crashWarner/exceptionHandler.hpp"
#include "signals/signalHandlers.hpp"
#include "signals/signals.hpp"
#include "suppression/firstPartyLibrary.hpp"
#include "trackers/TLSTracker.hpp"

#ifdef __APPLE__
extern "C" {
# include <mach/thread_state.h>
}

# include <CoreFoundation/CFDictionary.h>
# include <objc/runtime.h>

# define OBJC_SUPPORT_EXTRA 1
# include "objcSupport.hpp"
#endif

namespace lsan {
std::atomic_bool LSan::finished = false;
std::atomic_bool LSan::preventDealloc = false;

constexpr inline static auto alignment = sizeof(void*);

static constexpr inline auto align(uintptr_t ptr, const bool up = true) -> uintptr_t {
    if (ptr % alignment != 0) {
        if (up) {
            ptr = ptr + alignment - ptr % alignment;
        } else {
            ptr = ptr - alignment + ptr % alignment;
        }
    }
    return ptr;
}

static inline auto align(const void* ptr, const bool up = true) -> uintptr_t {
    return align(uintptr_t(ptr), up);
}

auto LSan::findWithSpecials(void* ptr) -> decltype(infos)::iterator {
    auto toReturn = infos.find(ptr);
    if (toReturn == infos.end()) {
        toReturn = infos.find(reinterpret_cast<void*>(uintptr_t(ptr) - 2 * sizeof(void*)));
    }
    if (toReturn == infos.end()) {
        toReturn = infos.find(reinterpret_cast<void*>(uintptr_t(ptr) - sizeof(void*)));
    }
    if (toReturn == infos.end()) {
        toReturn = infos.find(reinterpret_cast<void*>(~uintptr_t(ptr)));
    }
    return toReturn;
}

void LSan::classifyLeaks(const uintptr_t begin, const uintptr_t end,
                         const LeakType direct, const LeakType indirect,
                         std::deque<MallocInfo::Ref>& directs, const bool skipClassifieds,
                         const char* name, const char* nameRelative, const bool reclassify) {
    for (uintptr_t it = begin; it < end; it += sizeof(uintptr_t)) {
        const auto& record = findWithSpecials(*reinterpret_cast<void**>(it));
        if (record == infos.end() || record->second.deleted || (skipClassifieds && record->second.leakType != LeakType::unclassified)) {
            continue;
        }
        if (record->second.leakType > direct || reclassify) {
            record->second.leakType = direct;
            record->second.imageName.first = name;
            record->second.imageName.second = nameRelative;
            directs.emplace_back(record->second);
        }
        classifyRecord(record->second, indirect, reclassify);
    }
}

void LSan::classifyClass(void* cls, std::deque<MallocInfo::Ref>& directs, const LeakType direct, const LeakType indirect) {
    const auto classWords = static_cast<void**>(cls);
    const auto cachePtr = reinterpret_cast<void*>(uintptr_t(classWords[2]) & ((uintptr_t(1) << 48) - 1));
    if (const auto& cacheIt = infos.find(cachePtr); cacheIt != infos.end() && cacheIt->second.leakType > direct) {
        cacheIt->second.leakType = direct;
        classifyRecord(cacheIt->second, indirect);
        directs.emplace_back(cacheIt->second);
    }

    const auto ptr = reinterpret_cast<void*>(uintptr_t(classWords[4]) & 0x0f007ffffffffff8UL);
    if (const auto& it = infos.find(ptr); it != infos.end()) {
        if (it->second.leakType > direct) {
            it->second.leakType = direct;
            classifyRecord(it->second, indirect);
            directs.emplace_back(it->second);
        }

        const auto rwStuff = static_cast<void**>(it->second.pointer);
        const auto rwPtr = reinterpret_cast<void*>(uintptr_t(rwStuff[1]) & ~1);
        if (const auto& rwIt = infos.find(rwPtr); rwIt != infos.end()) {
            if (rwIt->second.leakType > direct) {
                rwIt->second.leakType = direct;
                classifyRecord(rwIt->second, indirect);
                directs.emplace_back(rwIt->second);
            }
            if (rwIt->second.size >= 4 * sizeof(void*)) {
                const auto ptrArr = static_cast<void**>(rwIt->second.pointer);
                for (unsigned char i = 1; i < 4; ++i) {
                    classifyPointerUnion<true>(ptrArr[i], directs, direct, indirect);
                }
            }
        }
    }
}

void LSan::classifyRecord(MallocInfo& info, const LeakType& currentType, const bool reclassify) {
    auto stack = std::stack<std::reference_wrapper<MallocInfo>>();
    stack.emplace(info);
    while (!stack.empty()) {
        auto& elem = stack.top();
        stack.pop();
        if ((elem.get().leakType > currentType || reclassify) && elem.get().pointer != info.pointer) {
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
            info.viaMeRecords.emplace_back(record->second);
            if (record->second.leakType > currentType || reclassify)
                stack.emplace(record->second);
        }
    }
}

static inline auto findStackBegin(pthread_t thread = pthread_self()) -> void* {
    void* toReturn;

#ifdef __APPLE__
    toReturn = pthread_get_stackaddr_np(thread);
#elif defined(__linux__)
    pthread_attr_t attr;
    std::size_t ignored;
    if (pthread_getattr_np(thread, &attr) != 0) {
        throw std::runtime_error("Failed to gather thread attributes");
    }
    if (pthread_attr_getstack(&attr, &toReturn, &ignored) != 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("Failed to gather stack address");
    }
    pthread_attr_destroy(&attr);
#endif

    return toReturn;
}

static inline auto findStackSize(pthread_t thread = pthread_self()) -> std::size_t {
    std::size_t toReturn;

#ifdef __APPLE__
    toReturn = pthread_get_stacksize_np(thread);
#elif defined(__linux__)
    pthread_attr_t attr;
    if (pthread_getattr_np(thread, &attr) != 0) {
        throw std::runtime_error("Failed to gather thread attributes");
    }
    if (pthread_attr_getstacksize(&attr, &toReturn) != 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("Failed to gather stack size");
    }
    pthread_attr_destroy(&attr);
#endif

    return toReturn;
}

/**
 * If the given pointer is a TLSTracker, it is deleted and the thread-local
 * value is set to point to the global tracker instance.
 *
 * @param value the thread-local value
 */
static inline void destroySaniKey(void* value) {
    if (auto& globalInstance = getInstance(); value != std::addressof(globalInstance)) {
        pthread_setspecific(globalInstance.saniKey, std::addressof(globalInstance));
        auto tracker = static_cast<trackers::ATracker*>(value);
        if (!LSan::preventDealloc) {
            globalInstance.withIgnoration(true, [&tracker] {
                delete tracker;
            });
        } else {
            tracker->needsDealloc = true;
        }
    }
}

auto LSan::isSuppressed(const MallocInfo& info) -> bool {
    if (suppression::isFirstParty(info.imageName.first, !callstack_autoClearCaches)) {
        return true;
    }

    const auto& suppressions = getSuppressions();
    return std::any_of(suppressions.cbegin(), suppressions.cend(), [&info](const auto& suppression) {
        return suppression.match(info);
    });
}

auto LSan::getThreadDescription(unsigned long id, const std::optional<pthread_t>& thread) -> const std::string& {
    using namespace std::string_literals;

    if (const auto& it = threadDescriptions.find(id); it != threadDescriptions.end()) {
        return it->second;
    }
    std::string desc;
    if (id == 0) {
        desc = "main thread";
    } else {
        desc = "thread # " + std::to_string(id);

        std::optional<pthread_t> t = thread;
        if (!t) {
            const auto& it = std::find_if(threads.cbegin(), threads.cend(), [id](const auto& element) {
                return
#ifdef __linux__
                    !element.second.isDead() &&
#endif
                    element.second.getNumber() == id;
            });
            if (it != threads.end()) {
                t = it->second.getThread();
            }
        }
        constexpr auto BUFFER_SIZE = 1024u;
        char buffer[BUFFER_SIZE];
        if (t && pthread_getname_np(*t, buffer, BUFFER_SIZE) == 0 && buffer[0] != '\0') {
            desc += " ("s + buffer + ")";
        }
    }
    return threadDescriptions.emplace(id, std::move(desc)).first->second;
}

static inline auto loadFunc(const char* name) -> void* {
    const auto result = functionInfo_load(name);
    return result.found ? reinterpret_cast<void*>(result.begin) : nullptr;
}

#ifdef __APPLE__
# define FUNC_NAME(name) "_" #name
#else
# define FUNC_NAME(name) #name
#endif

#define LOAD_FUNC(type, name) const auto name = reinterpret_cast<type>(loadFunc(FUNC_NAME(name)))

void LSan::classifyObjC(std::deque<MallocInfo::Ref>& records) {
#ifndef __APPLE__
    using id = void*;
    using Class = void*;
#endif

    LOAD_FUNC(int (*)(Class*, int), objc_getClassList);
    LOAD_FUNC(Class (*)(id), object_getClass);

    if (objc_getClassList == nullptr || object_getClass == nullptr) {
        return;
    }

    const auto& classNumber = objc_getClassList(nullptr, 0);
    const auto classes = new Class[classNumber];
    objc_getClassList(classes, classNumber);
    for (int i = 0; i < classNumber; ++i) {
        classifyClass(classes[i], records, LeakType::objcDirect, LeakType::objcIndirect);

        const auto& meta = object_getClass(id(classes[i]));
        classifyClass(meta, records, LeakType::objcDirect, LeakType::objcIndirect);
    }
    delete[] classes;
}

#ifdef __linux__
static std::thread::id killId;
volatile static bool holding = true;

static void holdOn(int) {
    if (std::this_thread::get_id() != killId) {
        while (holding); // FIXME: Use a condition variable
    }
}
#endif

static inline auto suspendThread(const ThreadInfo& info) -> bool {
    auto toReturn = false;
#ifdef __APPLE__
    toReturn = thread_suspend(pthread_mach_thread_np(info.getThread())) == KERN_SUCCESS;
#else
    killId = std::this_thread::get_id();
    toReturn = pthread_kill(info.getThread(), SIGUSR1) == 0;
#endif
    return toReturn;
}

static inline auto resumeThread(const ThreadInfo& info) -> bool {
    auto toReturn = false;
#ifdef __APPLE__
    toReturn = thread_resume(pthread_mach_thread_np(info.getThread())) == KERN_SUCCESS;
#else
    (void) info;
    toReturn = true;
#endif
    return toReturn;
}

static inline auto getStackPointer(const ThreadInfo& info) -> uintptr_t {
    uintptr_t toReturn;
    // TODO: Linux version
#ifdef __APPLE__
    auto count = std::size_t(0);
    if (thread_get_register_pointer_values(pthread_mach_thread_np(info.getThread()),
                                           &toReturn, &count, nullptr) != KERN_INSUFFICIENT_BUFFER_SIZE)
#endif
        toReturn = uintptr_t(info.getStackTop()) - info.getStackSize();
    return toReturn;
}

#ifdef __linux__
auto LSan::gatherPthreadSize() -> std::size_t {
    std::optional<ThreadInfo> info;
    for (const auto& [_, thread] : threads) {
        if (thread.getNumber() != 0 && !thread.isDead()) {
            info = thread;
            break;
        }
    }
    std::size_t toReturn;
    if (info) {
        toReturn = uintptr_t(info->getStackTop()) - uintptr_t(info->getThread());
    } else {
        std::thread([&toReturn]() {
            const auto& stackSize = findStackSize();
            const auto& stackBegin = findStackBegin();
            toReturn = uintptr_t(stackBegin) + stackSize - uintptr_t(pthread_self());
        }).join();
    }
    return toReturn;
}
#endif

auto LSan::classifyLeaks() -> LeakKindStats {
    auto toReturn = LeakKindStats();

    auto& out = getOutputStream();
    const auto& clear = [](std::ostream& stream) -> std::ostream& {
        if (isATTY()) {
            return stream << "\r                                                             \r";
        }
        return stream << std::endl;
    };
    out << "Searching globals and compile time thread locals...";
    const auto& [regions, regionsAmount] = regions_getLoadedRegions();

    out << clear << "Collecting the leaks...";
    for (auto it = infos.begin(); it != infos.end();) {
        if (it->second.deleted) {
            it = infos.erase(it);
        } else {
            ++it;
        }
    }

    out << clear << "Reachability analysis: Objective-C runtime...";
    classifyObjC(toReturn.recordsObjC);

    out << clear << "Reachability analysis: Stacks...";
#ifdef __linux__
    signal(SIGUSR1, holdOn);
#endif
    auto failed = std::vector<ThreadInfo>();
    for (const auto& [_, info] : threads) {
#ifdef __linux__
        if (info.isDead()) continue;
#endif

        using namespace formatter;

        const auto& threadDesc = getThreadDescription(info.getNumber(), info.getThread());

        const auto& selfThread = std::this_thread::get_id() == info.getId();
        if (!selfThread && !suspendThread(info)) {
            out << std::endl << format<Style::AMBER>("LSan: Warning: Failed to suspend " + threadDesc + ".") << std::endl;
            failed.push_back(info);
        }
        const auto& top = align(info.getStackTop(), false);
        const auto& sp = selfThread ? uintptr_t(__builtin_frame_address(0)) : getStackPointer(info);
        classifyLeaks(align(sp), top, LeakType::reachableDirect, LeakType::reachableIndirect,
                      toReturn.recordsStack, false, isThreaded ? threadDesc.c_str() : nullptr);
    }

    out << clear << "Reachability analysis: Globals...";
    // Search in global space
    for (std::size_t i = 0; i < regionsAmount; ++i) {
        const auto& [begin, end, name, nameRelative] = regions[i];

        classifyLeaks(align(begin), align(end, false),
                      LeakType::globalDirect, LeakType::globalIndirect,
                      toReturn.recordsGlobal, false, name, nameRelative);
    }

    out << clear << "Reachability analysis: Thread-locals...";
    for (const auto& [_, info] : threads) {
#ifdef __linux__
        if (info.isDead()) continue;
#endif

        const auto& threadDesc = isThreaded ? getThreadDescription(info.getNumber(), info.getThread()).c_str() : nullptr;

#ifdef __linux__
        const auto& end   = align(uintptr_t(info.getThread()) + gatherPthreadSize(), false);
        const auto& begin = align(end - 3744);
#else
        const auto& begin = align(uintptr_t(info.getThread()));
        const auto& end   = align(begin + __PTHREAD_SIZE__, false);
#endif
        classifyLeaks(begin, end, LeakType::tlvDirect, LeakType::tlvIndirect, toReturn.recordsTlv, false, threadDesc);
    }

    if (const auto& tlvSupp = createTLVSuppression(); !tlvSupp.empty()) {
        for (auto& [_, info] : infos) {
            if (const auto& infoForCXX17 = info; std::any_of(tlvSupp.cbegin(), tlvSupp.cend(), [&infoForCXX17](const auto& supp) {
                return supp.match(infoForCXX17);
            })) {
                classifyLeaks(align(info.pointer), align(uintptr_t(info.pointer) + info.size, false), LeakType::tlvDirect,
                              LeakType::tlvIndirect, toReturn.recordsTlv, false, nullptr, nullptr, true);
                info.suppressed = true;
            }
        }
    }

    for (const auto& [_, info] : threads) {
        if (info.getId() != std::this_thread::get_id() && std::find(failed.cbegin(), failed.cend(), info) == failed.end() && !resumeThread(info)) {
            using namespace formatter;

            out << std::endl << format<Style::AMBER>("LSan: Warning: Failed to resume "
                                                     + getThreadDescription(info.getNumber(), info.getThread()) + ".")
                << std::endl;
        }
    }
#ifdef __linux__
    holding = false;
#endif

#ifdef LSAN_HANDLE_OBJC
    out << clear << "Reachability analysis: Cocoa thread-local variables...";
    // Search in the Cocoa runtime thread-locals
    // TODO: Get dicts of all threads
    const auto& dict = CFDictionaryRef(_1(_2(NSThread, currentThread), threadDictionary));
    const auto& count = CFDictionaryGetCount(dict);
    const auto keys = new const void*[count];
    const auto values = new const void*[count];
    CFDictionaryGetKeysAndValues(dict, keys, values);
    for (CFIndex i = 0; i < count; ++i) {
        const auto& threadDesc = isThreaded ? getThreadDescription(getThreadId()).c_str() : nullptr;

        if (const auto& keyIt = infos.find(keys[i]); keyIt != infos.end()) {
            classifyRecord(keyIt->second, LeakType::tlvIndirect, true);
            keyIt->second.leakType = LeakType::tlvDirect;
            keyIt->second.imageName.first = threadDesc;
            toReturn.recordsTlv.emplace_back(keyIt->second);
        }
        if (const auto& valIt = infos.find(values[i]); valIt != infos.end()) {
            classifyRecord(valIt->second, LeakType::tlvIndirect, true);
            valIt->second.leakType = LeakType::tlvDirect;
            valIt->second.imageName.first = threadDesc;
            toReturn.recordsTlv.emplace_back(valIt->second);
        }
    }
    delete[] keys;
    delete[] values;

#endif

    out << clear << "Reachability analysis: Lost memory...";
    // All leaks still unclassified are unreachable, search for reachability inside them
    for (auto& [pointer, record] : infos) {
        if (record.leakType != LeakType::unclassified || record.deleted) {
            continue;
        }
        record.leakType = LeakType::unreachableDirect;
        classifyRecord(record, LeakType::unreachableIndirect);
        toReturn.recordsLost.emplace_back(record);
    }

    out << clear << "Filtering the memory leaks...";
    for (const auto& leak : toReturn.recordsObjC) {
        if (!leak.get().suppressed) {
            leak.get().markSuppressed();
        }
    }
    for (auto& [_, leak] : infos) {
        if (isSuppressed(leak) && !leak.suppressed) {
            leak.markSuppressed();
        }
    }

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

    out << clear;

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
    using namespace signals;

    atexit(exitHook);

    registerFunction(handlers::stats, SIGUSR1);
    
    registerFunction(asHandler(handlers::callstack), SIGUSR2, false);
    
    registerFunction(asHandler(handlers::crashWithTrace), SIGSEGV);
    registerFunction(asHandler(handlers::crashWithTrace), SIGABRT);
    registerFunction(asHandler(handlers::crashWithTrace), SIGTERM);
    registerFunction(asHandler(handlers::crashWithTrace), SIGALRM);
    registerFunction(asHandler(handlers::crashWithTrace), SIGPIPE);
    registerFunction(asHandler(handlers::crashWithTrace), SIGFPE);
    registerFunction(asHandler(handlers::crashWithTrace), SIGILL);
    registerFunction(asHandler(handlers::crashWithTrace), SIGQUIT);
    registerFunction(asHandler(handlers::crashWithTrace), SIGHUP);
    registerFunction(asHandler(handlers::crashWithTrace), SIGBUS);
    registerFunction(asHandler(handlers::crashWithTrace), SIGXFSZ);
    registerFunction(asHandler(handlers::crashWithTrace), SIGXCPU);
    registerFunction(asHandler(handlers::crashWithTrace), SIGSYS);
    registerFunction(asHandler(handlers::crashWithTrace), SIGVTALRM);
    registerFunction(asHandler(handlers::crashWithTrace), SIGPROF);
    registerFunction(asHandler(handlers::crashWithTrace), SIGTRAP);

#if defined(__APPLE__) || defined(SIGEMT)
    registerFunction(asHandler(handlers::crashWithTrace), SIGEMT);
#endif

    std::set_terminate(exceptionHandler);
}

LSan::~LSan() {
    for (const auto tracker : tlsTrackers) {
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

    const auto trackers = copyTrackerList();
    for (const auto tracker : trackers) {
        tracker->finish();
    }
}

void LSan::addThread() {
    addThread({
        findStackSize(),
        findStackBegin(),
        std::this_thread::get_id() == mainId ? 0 : ThreadInfo::createThreadId(),
        std::this_thread::get_id(),
        pthread_self(),
    });
    isThreaded = isThreaded || std::this_thread::get_id() != mainId;
}

void LSan::registerTracker(ATracker* tracker) {
    std::lock_guard lock1 { mutex };
    std::lock_guard lock { tlsTrackerMutex };

    withIgnoration(true, [&] {
        tlsTrackers.insert(tracker);
        addThread();
    });
}

void LSan::deregisterTracker(ATracker* tracker) {
    std::lock_guard lock1 { mutex };
    std::lock_guard lock { tlsTrackerMutex };

    withIgnoration(true, [&] {
        tlsTrackers.erase(tracker);
        removeThread();
    });
}

auto LSan::getThreadId(const std::thread::id& id) -> unsigned long {
    if (id == mainId) return 0;

    const auto& threadIt = threads.find(id);
    if (threadIt == threads.end()) {
        return std::numeric_limits<unsigned long>::max();
    }
    return threadIt->second.getNumber();
}

void LSan::absorbLeaks(PoolMap<const void *const, MallocInfo>&& leaks) {
    std::lock_guard lock { mutex };
    std::lock_guard lock1 { infoMutex };

    withIgnoration(true, [&] {
        infos.get_allocator().merge(leaks.get_allocator());
        infos.merge(std::move(leaks));
    });
}

// FIXME: Though unlikely, the invalidly freed record ref can become invalid throughout this process
auto LSan::removeMalloc(const ATracker* tracker, void* pointer) -> std::pair<bool, std::optional<MallocInfo::CRef>> {
    const auto& result = maybeRemoveMalloc(pointer);
    std::pair<bool, std::optional<MallocInfo::CRef>> tmp { false, std::nullopt };
    if (!result.first) {
        std::lock_guard lock { tlsTrackerMutex };
        for (const auto element : tlsTrackers) {
            if (element == tracker) continue;

            auto trackerResult = element->maybeRemoveMalloc(pointer);
            if (trackerResult.first) {
                return trackerResult;
            }
            if (!tmp.second || (tmp.second && trackerResult.second && trackerResult.second->get().isMoreRecent(tmp.second->get()))) {
                tmp = std::move(trackerResult);
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

void LSan::changeMalloc(const ATracker* tracker, MallocInfo&& info) {
    std::lock_guard lock { infoMutex };

    const auto& it = infos.find(info.pointer);
    if (it == infos.end()) {
        std::lock_guard tlsLock { tlsTrackerMutex };
        for (const auto element : tlsTrackers) {
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

void LSan::addThread(ThreadInfo&& info) {
#ifdef __linux__
    if (threads.find(info.getId()) != threads.end()) {
        return;
    }
#endif
    threads.insert_or_assign(info.getId(), info);
}

void LSan::removeThread(const std::thread::id& id) {
#ifdef __linux__
    threads.at(id).kill();
#else
    threads.erase(id);
#endif
}

auto LSan::getSuppressions() -> const std::vector<suppression::Suppression>& {
    if (!suppressions) {
        suppressions = loadSuppressions();
    }
    return *suppressions;
}

auto LSan::getSystemLibraries() -> const std::vector<std::regex>& {
    if (!systemLibraries) {
        systemLibraries = loadSystemLibraries();
    }
    return *systemLibraries;
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

static inline auto printIndirectHint(std::ostream& out) -> std::ostream& {
    using namespace formatter;

    out << "Hint: Set " << format<Style::BOLD>("LSAN_INDIRECT_LEAKS") << " to "
        << format<Style::BOLD>("true") << " to show indirect memory leaks." << std::endl << std::endl;
    return out;
}

static inline void printRecord(std::ostream& out, const MallocInfo& info) {
    const auto ptr = static_cast<void**>(info.pointer);
    for (std::size_t i = 0; i < info.size / 8; ++i) {
        out << ptr[i] << ", ";
    }
    out << std::endl << info.pointer << " ";
}

static inline auto printRecords(const std::deque<MallocInfo::Ref>& records, std::ostream& out, const LeakType allowed, bool printContent = false) -> bool {
    auto toReturn = false;
    for (const auto& leak : records) {
        if (auto& record = leak.get(); !record.printedInRoot && !record.suppressed && record.leakType == allowed) {
            if (printContent) {
                printRecord(out, record);
            }
            out << record << std::endl;
            record.printedInRoot = true;
            toReturn = true;
        }
    }
    return toReturn;
}

static inline auto operator<<(std::ostream& out, const LeakKindStats& stats) -> std::ostream& {
    using namespace formatter;

    // TODO: Maybe split between direct and indirect?
    out << format<Style::BOLD>("Summary:") << std::endl
        << "Total: " << stats.getTotal() << " leak" << (stats.getTotal() == 1 ? "" : "s")
                     << " (" << bytesToString(stats.getTotalBytes()) << ")" << std::endl
        << "       " << get<Style::BOLD> << stats.getTotalLost() << " leak" << (stats.getTotalLost() == 1 ? "" : "s")
                     << " (" << bytesToString(stats.getLostBytes()) << ") lost" << clear<Style::BOLD> << std::endl
        << "       " << stats.getTotalReachable() << " leak" << (stats.getTotalReachable() == 1 ? "" : "s")
                     << " (" << bytesToString(stats.getReachableBytes()) << ") reachable";
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
    auto printedLeaks = false;
    if (stats.getTotal() > 0) {
        // TODO: Optionally collapse identical callstacks
        stream << stats << std::endl;

        printedLeaks |= printRecords(stats.recordsLost, stream, LeakType::unreachableDirect);
        if (self.behaviour.showReachables()) {
            printedLeaks |= printRecords(stats.recordsGlobal, stream, LeakType::globalDirect);
            printedLeaks |= printRecords(stats.recordsTlv, stream, LeakType::tlvDirect);
            printedLeaks |= printRecords(stats.recordsStack, stream, LeakType::reachableDirect);
        } else if (stats.getTotalReachable() > 0) {
            stream << "Hint: Set " << format<Style::BOLD>("LSAN_SHOW_REACHABLES") << " to "
                   << format<Style::BOLD>("true") << " to display the reachable memory leaks."
                   << std::endl << std::endl;
        }

        if (self.callstackSizeExceeded) {
            stream << printCallstackSizeExceeded;
            self.callstackSizeExceeded = false;
        }
        if (!self.hadIndirects && printedLeaks) {
            stream << printIndirectHint;
        }
        if (printedLeaks && self.behaviour.relativePaths()) {
            stream << printWorkingDirectory;
        }
    } else {
        stream << format<Style::BOLD, Style::GREEN, Style::ITALIC>("No leaks detected.") << std::endl;
    }

    if (!isATTY() && !has("LSAN_PRINT_FORMATTED")) {
        stream << std::endl << "Hint: To re-enable colored output, set "
               << format<Style::BOLD>("LSAN_PRINT_FORMATTED") << " to "
               << format<Style::BOLD>("true") << "." << std::endl;
    }

    stream << maybeShowDeprecationWarnings;
    if (stats.getTotal() > 0 && printedLeaks) {
        stream << std::endl << stats;
    }

    callstack_clearCaches();
    callstack_autoClearCaches = true;
    
#ifdef BENCHMARK
    stream << std::endl << timing::printTimings << std::endl;
#endif

    return stream;
}
}
