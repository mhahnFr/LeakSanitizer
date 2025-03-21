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
#include <SimpleJSON/SimpleJSON.hpp>

#include "formatter.hpp"
#include "lsanMisc.hpp"

#include "callstacks/callstackHelper.hpp"

#include "suppression/defaultSuppression.hpp"
#include "suppression/FunctionNotFoundException.hpp"
#include "suppression/Suppression.hpp"

#include "trackers/PseudoTracker.hpp"
#include "trackers/TLSTracker.hpp"

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

static inline auto newLocalTracker(bool pseudo) -> trackers::ATracker* {
    if (pseudo) {
        return new trackers::PseudoTracker();
    }
    return new trackers::TLSTracker();
}

auto getTracker() -> trackers::ATracker& {
    auto& globalInstance = getInstance();
    if (globalInstance.finished) return globalInstance;

    const auto& key = globalInstance.saniKey;
    auto tlv = pthread_getspecific(key);
    if (tlv == nullptr) {
        pthread_setspecific(key, std::addressof(globalInstance));
        trackers::ATracker* tlsTracker;
        globalInstance.withIgnoration(true, [&] {
            tlsTracker = newLocalTracker(getBehaviour().statsActive());
            pthread_setspecific(key, tlsTracker);
        });
        return *tlsTracker;
    }
    return *static_cast<trackers::ATracker*>(tlv);
}

static inline auto getFiles(const char* files) -> std::vector<std::filesystem::path> {
    auto toReturn = std::vector<std::filesystem::path>();
    if (files != nullptr) {
        auto stream = std::istringstream(files);
        std::string s;
        while (std::getline(stream, s, ':')) {
            toReturn.push_back(s);
        }
    }
    return toReturn;
}

static inline void loadSuppressions(std::vector<suppression::Suppression>& content, const simple_json::Value& object) {
    if (object.is(simple_json::ValueType::Array)) {
        for (const auto& object : object.as<simple_json::ValueType::Array>()) {
            try {
                content.push_back(simple_json::Object(object));
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
        content.push_back(simple_json::Object(object));
    }
}

auto loadSuppressions() -> std::vector<suppression::Suppression> {
    auto toReturn = std::vector<suppression::Suppression>();
    for (const auto& file : suppression::getDefaultSuppression()) {
        try {
            loadSuppressions(toReturn, simple_json::parse(std::istringstream(file)));
        } catch (const std::exception& e) {
            using namespace formatter;
            using namespace std::string_literals;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load default suppression file: "s + e.what()) << std::endl << std::endl;
        }
    }

    for (const auto& file : getFiles(getBehaviour().suppressionFiles())) {
        auto stream = std::ifstream();
        stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

        try {
            stream.open(file);
            loadSuppressions(toReturn, simple_json::parse(stream));
        } catch (const std::exception& e) {
            using namespace formatter;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load suppression file \""
                                                                 + file.string() + "\": " + e.what()) << std::endl << std::endl;
        }
        if (stream.is_open()) {
            stream.close();
        }
    }

    return toReturn;
}

static inline void loadSystemLibraryFile(std::vector<std::regex>& content, const simple_json::Value& object) {
    using namespace simple_json;
    if (!object.is(ValueType::Array)) {
        throw std::runtime_error("System libraries should be defined as a top level string array");
    }
    for (const auto& value : object.as<ValueType::Array>()) {
        if (!value.is(ValueType::String)) {
            throw std::runtime_error("System library regex was not a string");
        }

        content.push_back(std::regex(value.as<ValueType::String>()));
    }
}

auto loadSystemLibraries() -> std::vector<std::regex> {
    auto toReturn = std::vector<std::regex>();

    for (const auto& file : suppression::getSystemLibraryFiles()) {
        try {
            loadSystemLibraryFile(toReturn, simple_json::parse(std::istringstream(file)));
        } catch (const std::exception& e) {
            using namespace formatter;
            using namespace std::string_literals;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load default system library file: "s + e.what()) << std::endl << std::endl;
        }
    }

    for (const auto& file : getFiles(getBehaviour().systemLibraryFiles())) {
        auto stream = std::ifstream();
        stream.exceptions(std::ifstream::badbit | std::ifstream::failbit);

        try {
            stream.open(file);
            loadSystemLibraryFile(toReturn, simple_json::parse(stream));
        } catch (const std::exception& e) {
            using namespace formatter;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load system library file \"" + file.string() + "\": " + e.what()) << std::endl << std::endl;
        }
        if (stream.is_open()) {
            stream.close();
        }
    }

    return toReturn;
}
}
