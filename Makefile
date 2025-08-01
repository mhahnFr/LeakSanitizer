#
# LeakSanitizer - Small library showing information about lost memory.
#
# Copyright (C) 2022 - 2025  mhahnFr
#
# This file is part of the LeakSanitizer.
#
# The LeakSanitizer is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The LeakSanitizer is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with the
# LeakSanitizer, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
#

CORE_NAME = liblsan

SHARED_L = $(CORE_NAME).so
DYLIB_NA = $(CORE_NAME).dylib

SIMPLE_JSON_DIR = ./SimpleJSON

LIBCALLSTACK_OPT  = true
LIBCALLSTACK_NAME = libcallstack
LIBCALLSTACK_DIR  = ./CallstackLibrary
LIBCALLSTACK_A    = $(LIBCALLSTACK_DIR)/$(LIBCALLSTACK_NAME).a
LIBCALLSTACK_FLAG = "CXX_FUNCTIONS=${LIBCALLSTACK_OPT}" 'USE_BUILTINS=false'

SRCS = \
	$(SIMPLE_JSON_DIR)/src/parser.cpp \
	src/timing.cpp \
	src/ThreadInfo.cpp \
	src/MallocInfo.cpp \
	src/lsanMisc.cpp \
	src/lsan_internals.cpp \
	src/LeakSani.cpp \
	src/bytePrinter.cpp \
	src/wrappers/wrap_malloc.cpp \
	src/wrappers/misc.cpp \
	src/trackers/TLSTracker.cpp \
	src/suppression/Suppression.cpp \
	src/suppression/firstPartyLibrary.cpp \
	src/suppression/defaultSuppression.cpp \
	src/statistics/Stats.cpp \
	src/statistics/lsan_stats.cpp \
	src/statistics/AutoStats.cpp \
	src/signals/signals.cpp \
	src/signals/signalHandlers.cpp \
	src/crashWarner/exceptionHandler.cpp \
	src/crashWarner/crashWarner.cpp \
	src/callstacks/callstackHelper.cpp \
	src/allocators/ObjectPool.cpp

OBJS = $(patsubst %.cpp, %.o, $(SRCS))
DEPS = $(patsubst %.cpp, %.d, $(SRCS))

SUPP_SRCS = \
	suppressions/linux/core.json \
	suppressions/linux/systemLibraries.json \
	suppressions/macos/AppKit.json \
	suppressions/macos/core.json \
	suppressions/macos/systemLibraries.json \
	suppressions/macos/tlv.json

SUPP_HS  = $(patsubst %.json, %.hpp, $(SUPP_SRCS))

DEFAULT_SUPP_CPP = src/suppression/defaultSuppression.cpp

LDFLAGS  = -L$(LIBCALLSTACK_DIR) -lcallstack
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -fPIC -I 'include' -I CallstackLibrary/include -I SimpleJSON/include -I suppressions -D__LSAN_SILENCE_DEPRECATION

LINUX_SONAME_FLAG = -Wl,-soname,$(abspath $@)
MACOS_ARCH_FLAGS =

ifeq ($(shell uname -s),Darwin)
	LDFLAGS += -current_version 1.10 -compatibility_version 1 -install_name $(abspath $@) $(MACOS_ARCH_FLAGS)
	CXXFLAGS += $(MACOS_ARCH_FLAGS) -O3 -ffast-math
	LIBCALLSTACK_FLAG += "MACOS_ARCH_FLAGS=$(MACOS_ARCH_FLAGS)"

	NAME = $(DYLIB_NA)
else
	LDFLAGS += $(LINUX_SONAME_FLAG) -ldl
	CXXFLAGS += -Ofast

	NAME = $(SHARED_L)
endif

VERSION = "clean build"
GIT_VERSION = $(shell git describe --tags --abbrev=1)
ifneq (GIT_VERSION,)
	VERSION = $(GIT_VERSION)
endif

INSTALL_PATH ?= /usr/local

default: $(NAME)

bench: CXXFLAGS += -DBENCHMARK
bench: default

debug: CXXFLAGS += -O0 -g
debug: default

all: $(SHARED_L) $(DYLIB_NA)

install:
	- $(RM) $(NAME)
	$(MAKE) LINUX_SONAME_FLAG="-Wl,-soname,$(NAME)" $(NAME)
	if [ "$(shell uname -s)" = "Darwin" ]; then install_name_tool -id "$(INSTALL_PATH)/lib/$(NAME)" $(NAME); fi
	mkdir -p $(INSTALL_PATH)/lib
	mkdir -p "$(INSTALL_PATH)/include/lsan"
	mv $(NAME) $(INSTALL_PATH)/lib
	find "include" -name \*.h -exec cp {} "$(INSTALL_PATH)/include/lsan" \;

uninstall:
	- $(RM) $(INSTALL_PATH)/lib/$(NAME)
	- $(RM) -r "$(INSTALL_PATH)/include/lsan"

release: clean
	$(MAKE) LINUX_SONAME_FLAG="-Wl,-soname,$(NAME)" MACOS_ARCH_FLAGS="-arch x86_64 -arch arm64 -arch arm64e" $(NAME)
	if [ "$(shell uname -s)" = "Darwin" ]; then install_name_tool -id "$(NAME)" $(NAME); fi

update:
	$(MAKE) clean
	git fetch --tags
	git pull
	git submodule update --init
	git -C $(LIBCALLSTACK_DIR) submodule update --init
	$(MAKE) re

$(SHARED_L): $(OBJS) $(LIBCALLSTACK_A)
	$(CXX) -shared -fPIC $(OBJS) $(LDFLAGS) -o $(SHARED_L)

$(DYLIB_NA): $(OBJS) $(LIBCALLSTACK_A)
	$(CXX) -dynamiclib $(LDFLAGS) -o $(DYLIB_NA) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DLSAN_VERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

%.hpp: %.json
	echo 'constexpr const char*' `echo $(basename $<) | tr /. _` '= R"lsanJsonLiteral(' > $@
	cat $< >> $@
	echo '\n)lsanJsonLiteral";' >> $@

$(DEFAULT_SUPP_CPP): $(SUPP_HS)

$(LIBCALLSTACK_A):
	$(MAKE) -C $(LIBCALLSTACK_DIR) $(LIBCALLSTACK_FLAG) $(LIBCALLSTACK_NAME).a

clean:
	- $(RM) $(OBJS) $(DEPS)
	- $(RM) $(SUPP_HS)
	- $(RM) $(SHARED_L) $(DYLIB_NA)
	- $(MAKE) -C $(LIBCALLSTACK_DIR) $(LIBCALLSTACK_FLAG) clean

re: clean
	$(MAKE) default

.PHONY: re clean all install uninstall release default update bench debug

-include $(DEPS)
