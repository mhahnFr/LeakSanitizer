/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2025  mhahnFr
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

#include <filesystem>
#include <fstream>
#include <sstream>

#if __has_include(<unistd.h>)
 #include <unistd.h>

 #define LSAN_HAS_UNISTD
#endif

#include <lsan_internals.h>
#include <callstack.h>

#include "lsanMisc.hpp"

#include "formatter.hpp"
#include "TLSTracker.hpp"
#include "callstacks/callstackHelper.hpp"

#include "suppression/defaultSuppression.hpp"
#include "suppression/FunctionNotFoundException.hpp"
#include "suppression/Suppression.hpp"
#include "suppression/json/Exception.hpp"
#include "suppression/json/parser.hpp"

#ifndef VERSION
 #define VERSION "clean build"
#endif

namespace lsan {
auto getInstance() -> LSan & {
    static auto instance = new LSan();
    return *instance;
}

/**
 * Prints the license information of this sanitizer.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
static inline auto printLicense(std::ostream & out) -> std::ostream & {
    out << "Copyright (C) 2022 - 2025  mhahnFr and contributors"         << std::endl
        << "Licensed under the terms of the GNU GPL version 3 or later." << std::endl
        << std::endl;
    
    return out;
}

/**
 * Prints the link to the website of this sanitizer.
 *
 * @param out the output stream to print to
 * @return the given output stream
 */
static inline auto printWebsite(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    out << formatter::get<Style::ITALIC>
        << "For more information, visit "
        << formatter::format<Style::UNDERLINED>("github.com/mhahnFr/LeakSanitizer")
        << formatter::clear<Style::ITALIC>
        << std::endl << std::endl;
    
    return out;
}

auto printInformation(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    out << "Report by " << formatter::format<Style::BOLD>("LeakSanitizer ")
        << formatter::format<Style::ITALIC>(VERSION)
        << std::endl << std::endl
        << printLicense
        << printWebsite;
    
    return out;
}

void exitHook() {
    using formatter::Style;

    getInstance().finish();
    getTracker().ignoreMalloc = true;
    getOutputStream() << maybePrintExitPoint
                      << std::endl     << std::endl
                      << getInstance() << std::endl
                      << printInformation;
    internalCleanUp();
}

auto maybeHintRelativePaths(std::ostream & out) -> std::ostream & {
    if (getBehaviour().relativePaths()) {
        out << printWorkingDirectory << std::endl;
    }
    return out;
}

auto printWorkingDirectory(std::ostream & out) -> std::ostream & {
    out << "Note: " << formatter::format<formatter::Style::GREYED>("Paths are relative to the") << " working directory: "
        << std::filesystem::current_path() << std::endl;
    
    return out;
}

auto isATTY() -> bool {
#ifdef LSAN_HAS_UNISTD
    return isatty(getBehaviour().printCout() ? STDOUT_FILENO : STDERR_FILENO);
#else
    return __lsan_printFormatted;
#endif
}

auto has(const std::string & var) -> bool {
    return getenv(var.c_str()) != nullptr;
}

auto maybePrintExitPoint(std::ostream& out) -> std::ostream& {
    using formatter::Style;

    if (getInstance().hasPrintedExit) return out;

    out << std::endl << formatter::format<Style::GREEN>("Exiting");
    if (getBehaviour().printExitPoint()) {
        out << formatter::format<Style::ITALIC>(", stacktrace:") << std::endl;
        callstackHelper::format(lcs::callstack(), out);
    }
    getInstance().hasPrintedExit = true;

    return out;
}

auto getTracker() -> ATracker& {
    auto& globalInstance = getInstance();
    if (globalInstance.finished || getBehaviour().statsActive()) return globalInstance;

    const auto& key = globalInstance.saniKey;
    auto tlv = pthread_getspecific(key);
    if (tlv == nullptr) {
        pthread_setspecific(key, std::addressof(globalInstance));
        TLSTracker* tlsTracker;
        {
            std::lock_guard lock(globalInstance.mutex);
            auto ignore = globalInstance.ignoreMalloc;
            globalInstance.ignoreMalloc = true;
            tlsTracker = new TLSTracker();
            pthread_setspecific(key, tlsTracker);
            globalInstance.ignoreMalloc = ignore;
        }
        return *tlsTracker;
    }
    return *static_cast<ATracker*>(tlv);
}

static inline auto getSuppressionFiles() -> std::vector<std::filesystem::path> {
    auto toReturn = std::vector<std::filesystem::path>();
    const auto& files = getBehaviour().suppressionFiles();
    if (files != nullptr) {
        auto stream = std::istringstream(files);
        std::string s;
        while (std::getline(stream, s, ':')) {
            toReturn.push_back(s);
        }
    }
    return toReturn;
}

static inline void loadSuppressions(std::vector<suppression::Suppression>& content, const json::Value& object) {
    if (object.is(json::ValueType::Array)) {
        for (const auto& object : object.as<json::ValueType::Array>()) {
            try {
                content.push_back(json::Object(object));
            } catch (const suppression::FunctionNotFoundException& e) {
                using namespace formatter;

                if (getBehaviour().suppressionDevelopersMode()) {
                    getOutputStream() << format<Style::BOLD, Style::RED>("LSan: Suppression \"" + e.getSuppressionName()
                                                                         + "\" ignored: Function \"" + e.getFunctionName()
                                                                         + "\" not loaded.") << std::endl << std::endl;
                }
            }
        }
    } else {
        content.push_back(json::Object(object));
    }
}

auto loadSuppressions() -> std::vector<suppression::Suppression> {
    auto toReturn = std::vector<suppression::Suppression>();
    for (const auto& file : suppression::getDefaultSuppression()) {
        try {
            loadSuppressions(toReturn, json::parse(std::istringstream(file)));
        } catch (const std::exception& e) {
            using namespace formatter;
            using namespace std::string_literals;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load default suppression file: "s + e.what()) << std::endl << std::endl;
        }
    }

    for (const auto& file : getSuppressionFiles()) {
        auto stream = std::ifstream();
        stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

        try {
            stream.open(file);
            loadSuppressions(toReturn, json::parse(stream));
        } catch (const std::exception& e) {
            using namespace std::string_literals;
            using namespace formatter;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load suppression file \""s
                                                                 + file.string() + "\": " + e.what()) << std::endl << std::endl;
        }
        if (stream.is_open()) {
            stream.close();
        }
    }

    return toReturn;
}
}
