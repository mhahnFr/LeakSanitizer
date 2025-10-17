/*
 * LeakSanitizer - Small library showing information about lost memory.
 *
 * Copyright (C) 2025  mhahnFr
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

#ifndef bundle_hpp
#define bundle_hpp

#include <filesystem>
#include <string>

#include <CoreFoundation/CFBundle.h>

/**
 * Groups everything related to the macOS bundle handling.
 */
namespace lsan::macos::bundle {
/**
 * @brief Returns the bundle associated with the LeakSanitizer.
 *
 * Needs to be paired with a call to @c killBundle() .
 *
 * @return the @c CFBundleRef
 */
auto getBundle() -> CFBundleRef;

/**
 * Deallocates the cached bundle information.
 */
void killBundle();

/**
 * Returns the path of the crash handler registered in the resources bundle.
 *
 * @return the path to the crash handler executable
 */
auto getCrashHandlerPath() -> std::string;

/**
 * Returns a version string.
 *
 * @return the version string
 */
auto getVersion() -> std::string;

/**
 * @brief Converts and returns the given CoreFoundation string reference to a C++ string.
 *
 * Returns an empty string if the convertion failed.
 *
 * @param str the @c CFStringRef to be converted
 * @return the @c std::string representation
 */
auto convertCFString(CFStringRef str) -> std::string;

namespace shared {
auto readFile(const std::filesystem::path& path) -> std::string;
auto loadResource(CFURLRef url) -> std::string;
}
}

#endif /* bundle_hpp */
