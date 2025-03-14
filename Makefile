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

LIBCALLSTACK_OPT  = true
LIBCALLSTACK_NAME = libcallstack
LIBCALLSTACK_DIR  = ./CallstackLibrary
LIBCALLSTACK_A    = $(LIBCALLSTACK_DIR)/$(LIBCALLSTACK_NAME).a
LIBCALLSTACK_FLAG = "CXX_FUNCTIONS=${LIBCALLSTACK_OPT}" 'USE_BUILTINS=false'

SRC   = $(shell find src -name \*.cpp \! -path $(LIBCALLSTACK_DIR)\*) SimpleJSON/src/parser.cpp
OBJS  = $(patsubst %.cpp, %.o, $(SRC))
DEPS  = $(patsubst %.cpp, %.d, $(SRC))

SUPP_SRC = $(shell find suppressions -name \*.json)
SUPP_HS  = $(patsubst %.json, %.hpp, $(SUPP_SRC))

DEFAULT_SUPP_CPP = src/suppression/defaultSuppression.cpp

BENCHMARK = false

LDFLAGS  = -L$(LIBCALLSTACK_DIR) -lcallstack
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -fPIC -Ofast -I 'include' -I CallstackLibrary/include -I SimpleJSON/include -I suppressions

ifeq ($(BENCHMARK),true)
	CXXFLAGS += -DBENCHMARK
endif

LINUX_SONAME_FLAG = -Wl,-soname,$(abspath $@)
MACOS_ARCH_FLAGS =

ifeq ($(shell uname -s),Darwin)
	LDFLAGS += -current_version 1.10 -compatibility_version 1 -install_name $(abspath $@) $(MACOS_ARCH_FLAGS) -lobjc -framework CoreFoundation
	CXXFLAGS += $(MACOS_ARCH_FLAGS) -DLSAN_HANDLE_OBJC
	LIBCALLSTACK_FLAG += "MACOS_ARCH_FLAGS=$(MACOS_ARCH_FLAGS)"

	NAME = $(DYLIB_NA)
else
	LDFLAGS += $(LINUX_SONAME_FLAG) -ldl

	NAME = $(SHARED_L)
endif

VERSION = "clean build"
ifneq ($(shell git describe --tags --abbrev=0),)
	VERSION = $(shell git describe --tags --abbrev=0)
endif

ifeq ($(shell ls $(LIBCALLSTACK_DIR)),)
	_  = $(shell git submodule init)
	_ += $(shell git submodule update)
endif

INSTALL_PATH ?= /usr/local

default: $(NAME)

bench:
	$(MAKE) 'BENCHMARK=true'

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
	git submodule update
	git -C $(LIBCALLSTACK_DIR) submodule update
	$(MAKE) re

$(SHARED_L): $(OBJS) $(LIBCALLSTACK_A)
	$(CXX) -shared -fPIC $(OBJS) $(LDFLAGS) -o $(SHARED_L)

$(DYLIB_NA): $(OBJS) $(LIBCALLSTACK_A)
	$(CXX) -dynamiclib $(LDFLAGS) -o $(DYLIB_NA) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DVERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

%.hpp: %.json
	echo 'constexpr const char*' `echo $(basename $<) | tr /. _` '= R"(' > $@
	cat $< >> $@
	echo '\n)";' >> $@

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

.PHONY: re clean all install uninstall release default update bench

-include $(DEPS)
