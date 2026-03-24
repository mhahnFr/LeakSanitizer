/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2023 - 2026  mhahnFr
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

#include "lsanMisc.hpp"

#include <callstack.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <SimpleJSON/SimpleJSON.hpp>

#include "formatter/formatter.hpp"
#include "formatter/lsanFormat.hpp"
#include "suppression/defaultSuppression.hpp"
#include "suppression/FunctionNotFoundException.hpp"
#include "suppression/Suppression.hpp"
#include "suppression/systemLibraryLoader.hpp"
#include "trackers/PseudoTracker.hpp"
#include "trackers/TLSTracker.hpp"

#ifndef LSAN_OS_DEFINED
# error Unknown operating system
#endif

#ifdef LSAN_OS_MACOS
# include "macos/bundle.hpp"
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

/**
 * Returns a string representing the current version of this project.
 *
 * @return the version string
 */
static inline auto getVersion() -> std::string {
#ifdef LSAN_OS_MACOS
    return macos::bundle::getVersion();
#elifdef LSAN_VERSION
    return LSAN_VERSION;
#else
    return "CLEAN BUILD";
#endif
}

auto printInformation(std::ostream & out) -> std::ostream & {
    using formatter::Style;
    
    out << "Report by " << formatter::format<Style::BOLD>("LeakSanitizer ")
        << formatter::format<Style::ITALIC>(getVersion())
        << std::endl << std::endl
        << printLicense
        << printWebsite;
    
    return out;
}

void exitHook() {
    getInstance().finish();
    if (shouldActivate()) [[likely]] {
        getTracker().ignoreMalloc = true;
        getOutputStream() << maybePrintExitPoint
        << std::endl     << std::endl
        << getInstance() << std::endl
        << printInformation;
    }
    internalCleanUp();
}

auto maybePrintExitPoint(std::ostream& out) -> std::ostream& {
    using formatter::Style;

    if (getInstance().hasPrintedExit) return out;

    out << std::endl << formatter::format<Style::GREEN>("Exiting");
    if (behaviour::getBehaviour().printExitPoint()) {
        out << formatter::format<Style::ITALIC>(", stacktrace:") << std::endl;
        callstack::format(lcs::callstack(), out);
    }
    getInstance().hasPrintedExit = true;

    return out;
}

/**
 * Creates a thread local tracker.
 *
 * @param pseudo whether to create a pseudo tracker
 * @return the new and allocated thread local tracker
 */
static inline auto newLocalTracker(const bool pseudo) -> trackers::ATracker* {
    if (pseudo) [[unlikely]] {
        return new trackers::PseudoTracker();
    }
    return new trackers::TLSTracker();
}

auto getTracker() -> trackers::ATracker& {
    auto& globalInstance = getInstance();
    if (LSan::finished) return globalInstance;

    const auto& key = globalInstance.getTlsKey();
    const auto tlv = pthread_getspecific(key);
    if (tlv == nullptr) {
        pthread_setspecific(key, std::addressof(globalInstance));
        trackers::ATracker* tlsTracker;
        globalInstance.withIgnoration(true, [&] {
            tlsTracker = newLocalTracker(behaviour::getBehaviour().statsActive());
            pthread_setspecific(key, tlsTracker);
        });
        return *tlsTracker;
    }
    return *static_cast<trackers::ATracker*>(tlv);
}

/**
 * Loads the suppressions found in the given JSON value into the given
 * suppression vector.
 *
 * @param content the vector with the deducted suppressions
 * @param object the JSON value to deduct suppressions from
 */
static inline void loadSuppressions(std::vector<suppression::Suppression>& content,
                                    const simple_json::Value& object) {
    if (object.is(simple_json::ValueType::Array)) {
        for (const auto& obj : object.as<simple_json::ValueType::Array>()) {
            try {
                content.emplace_back(simple_json::Object(obj));
            } catch (const suppression::FunctionNotFoundException& e) {
                using namespace formatter;

                if (behaviour::getBehaviour().suppressionDevelopersMode()) {
                    getOutputStream() << format<Style::BOLD, Style::RED>("LSan: Suppression \"" + e.getSuppressionName()
                                                                         + "\" ignored: Function \"" + e.getFunctionName()
                                                                         + "\" not loaded.") << std::endl << std::endl;
                }
            }
        }
    } else {
        content.emplace_back(simple_json::Object(object));
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

    for (const auto& file : behaviour::getFiles(behaviour::getBehaviour().suppressionFiles())) {
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

auto createTLVSuppression() -> std::vector<suppression::Suppression> {
    auto toReturn = std::vector<suppression::Suppression>();

    for (const auto& suppression : suppression::getDefaultTLVSuppressions()) {
        try {
            loadSuppressions(toReturn, simple_json::parse(std::istringstream(suppression)));
        } catch (const std::exception& e) {
            using namespace formatter;
            using namespace std::string_literals;

            getOutputStream() << format<Style::RED, Style::BOLD>("LSan: Failed to load TLV suppression: "s + e.what()) << std::endl << std::endl;
        }
    }

    return toReturn;
}

auto suppression::getSystemLibraries() -> const std::vector<std::regex>& {
    return LSan::getSystemLibraries();
}

auto shouldActivate() -> bool {
    return !has("\t\n\f\vLSAN_SUPPRESSED_BY_CRASH_HANDLER");
}
}