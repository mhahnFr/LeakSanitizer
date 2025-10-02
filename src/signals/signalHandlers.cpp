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

#include <array>
#include <string>

#ifdef __APPLE__
# define _XOPEN_SOURCE
# include <ucontext.h>
# undef _XOPEN_SOURCE

# include <unistd.h>
#endif /* __APPLE__ */

#include "signalHandlers.hpp"

#include <lsan_stats.h>

#include "SignalInfo.hpp"
#include "signals.hpp"
#include "../lsanMisc.hpp"
#include "../utils.hpp"
#include "../callstackHelper/format.hpp"
#include "../crashWarner/core.hpp"
#include "../crashWarner/crashForce.hpp"
#include "../formatter/formatter.hpp"

namespace lsan::signals::handlers {
/**
 * Creates a callstack using the pointer to the @c ucontext .
 *
 * @param ptr the pointer to the context for which to create a callstack for
 * @return the callstack
 */
static inline auto createCallstackFor(void* ptr) -> lcs::callstack {
    auto toReturn = lcs::callstack(false);
    
    /*
     * The Linux version of the following code has been deactivated because of
     * causing crashes when the frame pointer is unavailable.
     *
     * FIXME: Use .eh_frame unwinding information for this instead.
     *
     *                                                          - mhahnFr
     */
#if defined(__APPLE__) && (defined(__x86_64__) || defined(__i386__) || defined(__arm64__))
    const ucontext_t* context = static_cast<ucontext_t*>(ptr);
    
    uintptr_t ip, bp;
#ifdef __APPLE__
 #ifdef __x86_64__
    ip = context->uc_mcontext->__ss.__rip;
    bp = context->uc_mcontext->__ss.__rbp;
 #elif defined(__i386__)
    ip = context->uc_mcontext->__ss.__eip;
    bp = context->uc_mcontext->__ss.__ebp;
 #elif defined(__arm64__)
    ip = __darwin_arm_thread_state64_get_lr(context->uc_mcontext->__ss);
    bp = __darwin_arm_thread_state64_get_fp(context->uc_mcontext->__ss);
 #endif
#elif defined(__linux__)
 #ifdef __x86_64__
    ip = context->uc_mcontext.gregs[REG_RIP];
    bp = context->uc_mcontext.gregs[REG_RBP];
 #elif defined(__i386__)
    ip = context->uc_mcontext.gregs[REG_EIP];
    bp = context->uc_mcontext.gregs[REG_EBP];
 #endif
#endif

    const void* previousFrame = nullptr;
    auto frame         = reinterpret_cast<void*>(bp);
    auto returnAddress = reinterpret_cast<void*>(ip);

    auto addresses = std::array<void*, CALLSTACK_BACKTRACE_SIZE>();
    auto i = 0u;
    do {
        addresses[i++] = returnAddress;
        returnAddress = static_cast<void**>(frame)[1];
        previousFrame = frame;
        frame = *static_cast<void**>(frame);
    } while (frame > previousFrame && i < CALLSTACK_BACKTRACE_SIZE);
    toReturn = lcs::callstack(addresses.data(), int(i));
#else
    (void) ptr;
    toReturn = lcs::callstack();
#endif
    return toReturn;
}

[[ noreturn ]] static inline void crashWithTraceLocal(const int signalCode, const siginfo_t* signalContext, lcs::callstack&& callstack) {
    const auto [message, reasonDescription] = createCrashMessage(signalCode, signalContext->si_code, signalContext->si_addr);
    crashWarner::crashForce(message, reasonDescription, std::move(callstack));
}

#ifdef __APPLE__
[[ noreturn ]] static inline void crashWithTraceRemote(const int signalCode, const siginfo_t* signalContext, lcs::callstack&& callstack) {
    const auto& path = getInstance().crashHandlerPath;
    if (path.empty()) {
        crashWithTraceLocal(signalCode, signalContext, std::move(callstack));
    }
    if (const auto pid = fork(); pid < 0) {
        crashWithTraceLocal(signalCode, signalContext, std::move(callstack));
    } else if (pid == 0) {
        char buffer[sizeof(SignalInfo) + 1] {};
        char substitute[2] {};
        auto info = SignalInfo {
            .code = signalCode,
            .siCode = signalContext->si_code,
            .faultAddress = signalContext->si_addr,
            .callstack = std::move(callstack)
        };
        constexpr auto SIZE = CALLSTACK_BACKTRACE_SIZE + 4zu;
        const char* args[SIZE] = {
            path.c_str(),
        };
        args[SIZE - 1] = nullptr;
        info.callstack.relativize(args + 3);
        info.toBinary(buffer, sizeof buffer - 1, substitute);
        args[1] = substitute;
        args[2] = buffer;

        if (execv(path.c_str(), const_cast<char* const*>(args)) < 0) {
            crashWithTraceLocal(signalCode, signalContext, std::move(info.callstack));
        }
    } else {
        while (waitpid(pid, nullptr, WUNTRACED) != pid);
    }
    abort();
}
#endif

[[ noreturn ]] void crashWithTrace(const int signalCode, const siginfo_t* signalContext, void* executionContext) {
    getTracker().ignoreMalloc = true;
    auto callstack = createCallstackFor(executionContext);
#ifdef __APPLE__
    crashWithTraceRemote(signalCode, signalContext, std::move(callstack));
#else
    crashWithTraceLocal(signalCode, signalContext, std::move(callstack));
#endif
}

void callstack(int, siginfo_t*, void* executionContext) {
    using namespace formatter;

    getTracker().withIgnoration(true, [&executionContext] {
        auto& out = getOutputStream();
        out << format<Style::ITALIC>("The current callstack:") << std::endl;
        callstack::format(createCallstackFor(executionContext), out);
        out << std::endl;
    });
}

void stats(int) {
    lsan_printStats();
}
}
