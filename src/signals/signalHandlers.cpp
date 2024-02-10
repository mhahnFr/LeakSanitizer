/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2022 - 2024  mhahnFr
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

#include <array>
#include <iostream>
#include <optional>
#include <string>

#ifdef __APPLE__
 #define _XOPEN_SOURCE
 #include <ucontext.h>
 #undef _XOPEN_SOURCE
#endif /* __APPLE__ */

#include "signals.hpp"
#include "signalHandlers.hpp"

#include "../formatter.hpp"
#include "../lsanMisc.hpp"
#include "../MallocInfo.hpp"
#include "../callstacks/callstackHelper.hpp"
#include "../crashWarner/crash.hpp"

#include "../../include/lsan_internals.h"
#include "../../include/lsan_stats.h"

#include <sstream>

namespace lsan::signals::handlers {
static inline auto toString(void* ptr) -> std::string {
    std::stringstream stream;
    stream << ptr;
    return stream.str();
}

static inline auto createCallstackFor(void* ptr) -> lcs::callstack {
    auto toReturn = lcs::callstack(false);
    
#if defined(__APPLE__) && (defined(__x86_64__) || defined(__i386__))
    const ucontext_t* context = reinterpret_cast<ucontext_t*>(ptr);
    
    size_t ip, bp;
#ifdef __x86_64__
    ip = context->uc_mcontext->__ss.__rip;
    bp = context->uc_mcontext->__ss.__rbp;
#elif defined(__i386__)
    ip = context->uc_mcontext->__ss.__eip;
    bp = context->uc_mcontext->__ss.__ebp;
#endif
    
    void** frameBasePointer           = reinterpret_cast<void**>(bp);
    void*  extendedInstructionPointer = reinterpret_cast<void*>(ip);

    auto addresses = std::array<void*, CALLSTACK_BACKTRACE_SIZE>();
    std::size_t i = 0;
    do {
        addresses[i++] = extendedInstructionPointer;
        
        extendedInstructionPointer = frameBasePointer[1];
        frameBasePointer = reinterpret_cast<void**>(frameBasePointer[0]);
    } while (extendedInstructionPointer != nullptr);
    
    toReturn = lcs::callstack(addresses.data(), static_cast<int>(i));
#elif defined(__APPLE__) && (defined(__arm64__) || defined(__arm__))
    // TODO: Properly implement
    toReturn = lcs::callstack();
#else
    toReturn = lcs::callstack();
#endif
    return toReturn;
}

static inline auto getReasonSEGV(int code) -> std::optional<std::string> {
    switch (code) {
        case SEGV_MAPERR: return "Address not existent";
        case SEGV_ACCERR: return "Access to address denied";
    }
    return std::nullopt;
}

static inline auto getReasonILL(int code) -> std::optional<std::string> {
    switch (code) {
        case ILL_ILLOPC: return "Illegal opcode";
        case ILL_ILLTRP: return "Illegal trap";
        case ILL_PRVOPC: return "Privileged opcode";
        case ILL_ILLOPN: return "Illegal operand";
        case ILL_ILLADR: return "Illegal addressing mode";
        case ILL_PRVREG: return "Privileged register";
        case ILL_COPROC: return "Coprocessor error";
        case ILL_BADSTK: return "Internal stack error";
    }
    return std::nullopt;
}

static inline auto getReasonFPE(int code) -> std::optional<std::string> {
    switch (code) {
        case FPE_FLTDIV: return "Floating point divide by zero";
        case FPE_FLTOVF: return "Floating point overflow";
        case FPE_FLTUND: return "Floating point underflow";
        case FPE_FLTRES: return "Floating point inexact result";
        case FPE_FLTINV: return "Invalid floating point operation";
        case FPE_FLTSUB: return "Subscript out of range";
        case FPE_INTDIV: return "Integer divide by zero";
        case FPE_INTOVF: return "Integer overflow";
    }
    return std::nullopt;
}

static inline auto getReasonBUS(int code) -> std::optional<std::string> {
    switch (code) {
        case BUS_ADRALN: return "Invalid address alignment";
        case BUS_ADRERR: return "Physical address not existent";
        case BUS_OBJERR: return "Object-specific HW error";
    }
    return std::nullopt;
}

static inline auto getReasonTRAP(int code) -> std::optional<std::string> {
    switch (code) {
        case TRAP_BRKPT: return "Process breakpoint";
        case TRAP_TRACE: return "Process trace trap";
    }
    return std::nullopt;
}

static inline auto getReason(int signalCode, int code) -> std::optional<std::string> {
    using formatter::Style;
    
    switch (signalCode) {
        case SIGSEGV: return getReasonSEGV(code);
        case SIGILL:  return getReasonILL(code);
        case SIGFPE:  return getReasonFPE(code);
        case SIGBUS:  return getReasonBUS(code);
        case SIGTRAP: return getReasonTRAP(code);
    }
    
    switch (code) {
        case SI_USER:  return "Sent by " + formatter::formatString<Style::BOLD>("kill")     + "(2)";
        case SI_QUEUE: return "Sent by " + formatter::formatString<Style::BOLD>("sigqueue") + "(3)";
        case SI_TIMER: return "POSIX timer expired";
        case SI_MESGQ: return "POSIX message queue state changed";
            
#ifdef SI_TKILL
        case SI_TKILL: return formatter::formatString<Style::BOLD>("tkill") + "(2) or " + formatter::formatString<Style::BOLD>("tgkill") + "(2)";
#endif
#ifdef SI_KERNEL
        case SI_KERNEL: return "Sent by the kernel";
#endif
    }
    
    return std::nullopt;
}

static inline auto stringifyReasonSEGV(const int code) -> std::optional<std::string> {
    switch (code) {
        case SEGV_ACCERR: return "ACCERR";
        case SEGV_MAPERR: return "MAPERR";
    }
    return std::nullopt;
}

static inline auto stringifyReasonILL(const int code) -> std::optional<std::string> {
    switch (code) {
        case ILL_ILLOPC: return "ILLOPC";
        case ILL_ILLTRP: return "ILLTRP";
        case ILL_PRVOPC: return "PRVOPC";
        case ILL_ILLOPN: return "ILLOPN";
        case ILL_ILLADR: return "ILLADR";
        case ILL_PRVREG: return "PRVREG";
        case ILL_COPROC: return "COPROC";
        case ILL_BADSTK: return "BADSTK";
    }
    return std::nullopt;
}

static inline auto stringifyReasonFPE(const int code) -> std::optional<std::string> {
    switch (code) {
        case FPE_FLTDIV: return "FLTDIV";
        case FPE_FLTOVF: return "FLTOVF";
        case FPE_FLTUND: return "FLTUND";
        case FPE_FLTRES: return "FLTRES";
        case FPE_FLTINV: return "FLTINV";
        case FPE_FLTSUB: return "FLTSUB";
        case FPE_INTDIV: return "INTDIV";
        case FPE_INTOVF: return "INTOVF";
    }
    return std::nullopt;
}

static inline auto stringifyReasonBUS(const int code) -> std::optional<std::string> {
    switch (code) {
        case BUS_ADRALN: return "ADRALN";
        case BUS_ADRERR: return "ADRERR";
        case BUS_OBJERR: return "OBJERR";
    }
    return std::nullopt;
}

static inline auto stringifyReasonTRAP(const int code) -> std::optional<std::string> {
    switch (code) {
        case TRAP_BRKPT: return "BRKPT";
        case TRAP_TRACE: return "TRACE";
    }
    return std::nullopt;
}

static inline auto stringifyReason(const int signalCode, const int code) -> std::optional<std::string> {
    switch (signalCode) {
        case SIGSEGV: return stringifyReasonSEGV(code);
        case SIGILL:  return stringifyReasonILL(code);
        case SIGFPE:  return stringifyReasonFPE(code);
        case SIGBUS:  return stringifyReasonBUS(code);
        case SIGTRAP: return stringifyReasonTRAP(code);
    }
    
    switch (code) {
        case SI_USER:  return "SI_USER";
        case SI_QUEUE: return "SI_QUEUE";
        case SI_TIMER: return "SI_TIMER";
        case SI_MESGQ: return "SI_MESGQ";
            
#ifdef SI_TKILL
        case SI_TKILL: return "SI_TKILL";
#endif
#ifdef SI_KERNEL
        case SI_KERNEL: return "SI_KERNEL";
#endif
    }
    
    return std::nullopt;
}

[[ noreturn ]] void crashWithTrace(int signalCode, siginfo_t* info, void* ptr) {
    using formatter::Style;
        
    const auto reason = getReason(signalCode, info->si_code);
    crashForce(formatter::formatString<Style::BOLD, Style::RED>(getDescriptionFor(signalCode))
               + " (" + stringify(signalCode) + ")"
               + (hasAddress(signalCode) ? " on address " + formatter::formatString<Style::BOLD>(toString(info->si_addr)) : ""),
               reason.has_value()
                ? std::optional(formatter::formatString<Style::RED>(*reason) + " (" + stringifyReason(signalCode, info->si_code).value_or("Unknown reason") + ")")
                : std::nullopt,
               createCallstackFor(ptr));
}

void callstack(int, siginfo_t*, void* context) {
    using formatter::Style;
    
    bool ignore = getIgnoreMalloc();
    setIgnoreMalloc(true);
    
    auto& out = getOutputStream();
    out << formatter::format<Style::ITALIC>("The current callstack:") << std::endl;
    callstackHelper::format(createCallstackFor(context), out);
    out << std::endl;
    
    if (!ignore) {
        setIgnoreMalloc(false);
    }
}

void stats(int) {
    __lsan_printStats();
}
}
