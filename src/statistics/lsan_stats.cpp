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

#include <cmath>
#include <functional>
#include <iostream>

#include <lsan_stats.h>

#include "../bytePrinter.hpp"
#include "../formatter.hpp"
#include "../LeakSani.hpp"
#include "../lsanMisc.hpp"

using namespace lsan;

auto __lsan_getTotalMallocs() -> std::size_t { return getStats().getTotalMallocCount(); }
auto __lsan_getTotalBytes()   -> std::size_t { return getStats().getTotalBytes();       }
auto __lsan_getTotalFrees()   -> std::size_t { return getStats().getTotalFreeCount();   }

auto __lsan_getCurrentMallocCount() -> std::size_t { return getStats().getCurrentMallocCount(); }
auto __lsan_getCurrentByteCount()   -> std::size_t { return getStats().getCurrentBytes();       }

auto __lsan_getMallocPeek() -> std::size_t { return getStats().getMallocPeek(); }
auto __lsan_getBytePeek()   -> std::size_t { return getStats().getBytePeek();   }

namespace lsan {
/** The color for the bar elements to be used. */
static constexpr inline auto BAR_COLOR = formatter::Style::CYAN;

/**
 * @brief Prints the statistics using the given parameters.
 *
 * The given functions are responsible for printing the bar.
 *
 * @param statsName the name of the statistics, printed at the beginning
 * @param width the width in characters that the printed bar should have, passed
 * to the bar printing functions
 * @param out the stream to which to print
 * @param printBarBytes a function printing a bar for the byte part of the stats,
 * it gets the width and the output stream as parameters
 * @param printBarObjects a function printing a bar for the object part of the
 * stats, it gets the width and the output stream as parameters
 */
static inline void printStatsCore(const std::string & statsName, const std::size_t width, std::ostream & out,
                                  const std::function<void (std::size_t, std::ostream &)>& printBarBytes,
                                  const std::function<void (std::size_t, std::ostream &)>& printBarObjects) {
    using formatter::Style;
    out << formatter::format<Style::ITALIC>("Stats of the " + statsName + " so far:") << std::endl;
    
    out << formatter::clearAll()
        << __lsan_getCurrentMallocCount() << " objects in the heap, peek " << __lsan_getMallocPeek() << ", " << __lsan_getTotalFrees() << " deleted objects."
        << std::endl << std::endl;
    
    out << formatter::format<Style::BOLD>(bytesToString(__lsan_getCurrentByteCount()))
        << " currently used, peek " << bytesToString(__lsan_getBytePeek()) << "." << std::endl;
    printBarBytes(width, out);
    
    out << formatter::get<Style::BOLD>
        << __lsan_getCurrentMallocCount() << " objects"
        << formatter::clear<Style::BOLD>
        << " currently in the heap, peek " << __lsan_getMallocPeek() << " objects." << std::endl;
    printBarObjects(width, out);
}

/**
 * @brief A function that prints a bar using the given parameters.
 *
 * @param current the current amount
 * @param peek the peek amount
 * @param width the width in characters the bar should have
 * @param peekText the text to printed as peek, immediately after the bar
 * @param out the output stream to print to
 */
static inline void printBar(const std::size_t   current,
                            const std::size_t   peek,
                            const std::size_t   width,
                            const std::string & peekText,
                            std::ostream &      out) {
    using formatter::Style;
    
    out << formatter::format<Style::BOLD>("[")
        << formatter::get<BAR_COLOR, Style::UNDERLINED>;

    std::size_t i;
    for (i = 0; i < std::size_t(double(current) / double(peek) * double(width)); ++i) {
        out << formatter::get<Style::BAR_FILLED>;
    }
    for (; i < width; ++i) {
        out << formatter::get<Style::BAR_EMPTY>;
    }
    out << formatter::clear<BAR_COLOR, Style::UNDERLINED>
        << formatter::format<Style::BOLD>("]") << " of " << formatter::format<Style::BOLD>(peekText) << " peek"
        << std::endl << std::endl;
}

/**
 * @brief Prints a bar representing the fragmentation of the allocated objects.
 *
 * @param width the width in characters the bar should have
 * @param out the output stream to print to
 */
static inline void printFragmentationObjectBar(const std::size_t width, std::ostream & out) {
    using formatter::Style;
    
    out << formatter::format<Style::BOLD>("[")
        << formatter::get<BAR_COLOR, Style::UNDERLINED>;

    std::lock_guard lock(getInstance().getInfoMutex());
    
    const auto & infos = getInstance().getFragmentationInfos();
    auto it = infos.cbegin();
    if (infos.size() < width) {
        const double step = double(width) / double(infos.size()),
                     loss = fmod(step, double(int(step)));
        double tmpLoss = 0.0f;
        for (; it != infos.cend(); ++it) {
            const std::string& fill = it->second.isDeleted() ? formatter::get<Style::BAR_EMPTY>()
                                                             : formatter::get<Style::BAR_FILLED>();
            tmpLoss += loss;
            if (tmpLoss >= 1.0f) {
                out << fill;
                tmpLoss -= 1.0f;
            }
            for (std::size_t i = 0; i < static_cast<std::size_t>(step); ++i) {
                out << fill;
            }
        }
    } else {
        const double step = double(infos.size()) / double(width),
                    loss = fmod(step, double(long(step)));
        double tmpLoss = 0.0f;
        
        bool previousFilled    = false,
             previousCorrected = false;
        
        std::size_t previousFs = 0;
        for (std::size_t i = 0; i < width; ++i) {
            auto e = std::next(it, int(step));
            tmpLoss += loss;
            bool corrected = false;
            if (tmpLoss >= 1.0f) {
                ++e;
                tmpLoss -= 1.0f;
                corrected = true;
            }
            std::size_t fs = 0;
            for (; it != e; ++it) {
                if (it->second.isDeleted()) {
                    ++fs;
                }
            }
            const bool compare = corrected && !previousCorrected ?
                                    fs - 1 < previousFs
                                 : !corrected && previousCorrected ?
                                    fs < previousFs - 1
                                 :
                                    fs < previousFs;
            if (!previousFilled && compare) {
                out << formatter::get<Style::BAR_FILLED>;
                previousFilled = true;
            } else if (double(fs) < step / 2.0f) {
                if (previousFilled && fs > previousFs) {
                    out << formatter::get<Style::BAR_EMPTY>;
                    previousFilled = false;
                } else {
                    out << formatter::get<Style::BAR_FILLED>;
                    previousFilled = true;
                }
            } else {
                out << formatter::get<Style::BAR_EMPTY>;
                previousFilled = false;
            }
            previousFs = fs;
            previousCorrected = corrected;
        }
    }
    out << formatter::clear<BAR_COLOR, Style::UNDERLINED>
        << formatter::format<Style::BOLD>("]") << " of "
        << formatter::get<Style::BOLD> << infos.size() << " objects"
        << formatter::clear<Style::BOLD> << " total" << std::endl << std::endl;
}

/**
 * @brief Prints a bar representing the fragmentation of the allocated bytes.
 *
 * @param width the width in characters the bar should have
 * @param out the output stream to print to
 */
static inline void printFragmentationByteBar(const std::size_t width, std::ostream & out) {
    using formatter::Style;
    
    out << formatter::format<Style::BOLD>("[")
        << formatter::get<BAR_COLOR, Style::UNDERLINED>;

    std::lock_guard lock(getInstance().getInfoMutex());
    
    const auto & infos = getInstance().getFragmentationInfos();
    auto it = infos.cbegin();
    std::size_t currentBlockBegin = 0,
                currentBlockEnd   = it->second.getSize(),
                b                 = 0;
    
    auto total = std::size_t(0);
    for (const auto & [_, info] : infos) {
        total += info.getSize();
    }
    
    if (total < width) {
        const auto step = std::size_t(double(width) / double(total));
        for (; b < total; ++b) {
            if (b >= currentBlockEnd) {
                ++it;
                currentBlockBegin = b;
                currentBlockEnd   = currentBlockBegin + it->second.getSize();
            }
            const std::string& fill = it->second.isDeleted() ? formatter::get<Style::BAR_EMPTY>()
                                                             : formatter::get<Style::BAR_FILLED>();
            for (std::size_t i = 0; i < step; ++i) {
                out << fill;
            }
        }
    } else {
        const double step = double(total) / double(width),
                     loss = fmod(step, double(long(step)));
        double tmpLoss = 0.0f;
        
        bool previousFilled    = false,
             previousCorrected = false;
        
        std::size_t previousFs = 0;
        for (std::size_t i = 0; i < width; ++i) {
            bool corrected = false;
            auto tmpStep = std::size_t(step);
            tmpLoss += loss;
            if (tmpLoss >= 1.0f) {
                tmpLoss -= 1.0f;
                ++tmpStep;
                corrected = true;
            }
            std::size_t fs = 0;
            for (std::size_t j = 0; j < tmpStep && b < total; ++j, ++b) {
                if (b >= currentBlockEnd) {
                    ++it;
                    currentBlockBegin = b;
                    currentBlockEnd   = currentBlockBegin + it->second.getSize();
                }
                if (it->second.isDeleted()) {
                    ++fs;
                }
            }
            const bool compare = corrected && !previousCorrected ?
                                    fs - 1 < previousFs
                                 : !corrected && previousCorrected ?
                                    fs < previousFs - 1
                                 :
                                    fs < previousFs;
            if (!previousFilled && compare) {
                out << formatter::get<Style::BAR_FILLED>;
                previousFilled = true;
            } else if (double(fs) < step / 2.0f) {
                if (previousFilled && fs > previousFs) {
                    out << formatter::get<Style::BAR_EMPTY>;
                    previousFilled = false;
                } else {
                    out << formatter::get<Style::BAR_FILLED>;
                    previousFilled = true;
                }
            } else {
                out << formatter::get<Style::BAR_EMPTY>;
                previousFilled = false;
            }
            previousFs = fs;
            previousCorrected = corrected;
        }
    }
    out << formatter::clear<BAR_COLOR, Style::UNDERLINED>
        << formatter::format<Style::BOLD>("]") << " of "
        << formatter::format<Style::BOLD>(bytesToString(total)) << " total"
        << std::endl << std::endl;
}
}

void __lsan_printFragmentationStatsWithWidth(const std::size_t width) {
    using namespace formatter;

    getTracker().withIgnoration(true, [=] {
        auto& out = getOutputStream();
        if (getBehaviour().statsActive()) {
            printStatsCore("memory fragmentation", width, out,
                                  printFragmentationByteBar,
                                  printFragmentationObjectBar);
        } else {
            out << get<Style::RED>
                << format<Style::BOLD>("No memory fragmentation stats available at the moment!")
                << std::endl
                << format<Style::ITALIC>("Hint: Did you set ")
                << clear<Style::RED>
                << "LSAN_STATS_ACTIVE (" << format<Style::GREYED>("__lsan_statsActive") << ")"
                << format<Style::ITALIC, Style::RED>(" to ")
                << "true" << format<Style::RED, Style::ITALIC>("?")
                << std::endl << std::endl;
        }
    });
}

void __lsan_printStatsWithWidth(const std::size_t width) {
    using namespace formatter;

    getTracker().withIgnoration(true, [=] {
        auto& out = getOutputStream();
        if (getBehaviour().statsActive()) {
            printStatsCore("memory usage", width, out,
                                  [count = __lsan_getCurrentByteCount(), peek = __lsan_getBytePeek(),
                                      byteStr = bytesToString(__lsan_getBytePeek())] (auto&& theWidth, auto&& stream) {
                                      printBar(count, peek, std::forward<decltype(theWidth)>(theWidth), byteStr, std::forward<decltype(stream)>(stream));
                                  },
                                  [count = __lsan_getCurrentMallocCount(), peek = __lsan_getMallocPeek(),
                                      objectsStr = std::to_string(__lsan_getMallocPeek()) + " objects"] (auto&& theWidth, auto&& stream) {
                                      printBar(count, peek, std::forward<decltype(theWidth)>(theWidth), objectsStr, std::forward<decltype(stream)>(stream));
                                  });
        } else {
            out << get<Style::RED>
                << format<Style::BOLD>("No memory statistics available at the moment!") << std::endl
                << format<Style::ITALIC>("Hint: Did you set ")
                << clear<Style::RED>
                << "LSAN_STATS_ACTIVE (" << format<Style::GREYED>("__lsan_statsActive") << ")"
                << format<Style::ITALIC, Style::RED>(" to ")
                << "true" << format<Style::RED, Style::ITALIC>("?")
                << std::endl << std::endl;
        }
    });
}
