#
# LeakSanitizer - Small library showing information about lost memory.
#
# Copyright (C) 2022 - 2023  mhahnFr
#
# This file is part of the LeakSanitizer. This library is free software:
# you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this library, see the file LICENSE.  If not, see <https://www.gnu.org/licenses/>.
#

CORE_NAME = liblsan

SHARED_L = $(CORE_NAME).so
DYLIB_NA = $(CORE_NAME).dylib

LIBCALLSTACK_NAME = libcallstack
LIBCALLSTACK_DIR  = ./CallstackLibrary
LIBCALLSTACK_A    = $(LIBCALLSTACK_DIR)/$(LIBCALLSTACK_NAME).a
LIBCALLSTACK_SO   = $(LIBCALLSTACK_DIR)/$(LIBCALLSTACK_NAME).so
LIBCALLSTACK_DY   = $(LIBCALLSTACK_DIR)/$(LIBCALLSTACK_NAME).dylib
LIBCALLSTACK_FLAG = 'CXX_DEMANGLER=true'

SRC   = $(shell find code -name \*.cpp \! -path $(LIBCALLSTACK_DIR)\*)
SRC_C = $(shell find code -name \*.c \! -path $(LIBCALLSTACK_DIR)\*)
OBJS  = $(patsubst %.cpp, %.o, $(SRC)) $(patsubst %.c, %.o, $(SRC_C))
DEPS  = $(patsubst %.cpp, %.d, $(SRC)) $(patsubst %.c, %.d, $(SRC_C))

LDFLAGS = -ldl -L$(LIBCALLSTACK_DIR) -lcallstack
CXXFLAGS = -std=c++17 -Wall -pedantic -fPIC -Ofast
CFLAGS = -std=gnu11 -Wall -Wextra -fPIC -Ofast

ifeq ($(shell uname -s),Darwin)
 	LDFLAGS += -current_version 1.6 -compatibility_version 1 -install_name $(abspath $@)
 	
 	NAME = $(DYLIB_NA)
else
	NAME = $(SHARED_L)
endif

NO_LICENSE = false
ifeq ($(NO_LICENSE),true)
	CXXFLAGS += -DNO_LICENSE
endif

NO_WEBSITE = false
ifeq ($(NO_WEBSITE),true)
	CXXFLAGS += -DNO_WEBSITE
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

all: $(SHARED_L) $(DYLIB_NA)

install:
	$(MAKE) NO_LICENSE=false NO_WEBSITE=false $(NAME)
	if [ "$(shell uname -s)" = "Darwin" ]; then install_name_tool -id "$(INSTALL_PATH)/lib/$(NAME)" $(NAME); fi
	mkdir -p $(INSTALL_PATH)/lib
	mkdir -p "$(INSTALL_PATH)/include"
	cp $(NAME) $(INSTALL_PATH)/lib
	find "include" -name \*.h -exec cp {} "$(INSTALL_PATH)/include" \;

uninstall:
	- $(RM) $(INSTALL_PATH)/lib/$(NAME)
	- $(RM) $(addprefix $(INSTALL_PATH)/, $(shell find "include" -name \*.h))

$(SHARED_L): $(OBJS) $(LIBCALLSTACK_SO)
	$(CXX) -shared -fPIC $(LDFLAGS) -o $(SHARED_L) $(OBJS) $(LIBCALLSTACK_A)

$(DYLIB_NA): $(OBJS) $(LIBCALLSTACK_DY)
	$(CXX) -dynamiclib $(LDFLAGS) -o $(DYLIB_NA) $(OBJS) $(LIBCALLSTACK_A)

%.o: %.c
	$(CC) $(CFLAGS) -DVERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DVERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

$(LIBCALLSTACK_A):
	$(MAKE) -C $(LIBCALLSTACK_DIR) $(LIBCALLSTACK_FLAG) $(LIBCALLSTACK_NAME).a

clean:
	- $(RM) $(OBJS) $(DEPS)
	- $(MAKE) -C $(LIBCALLSTACK_DIR) clean

fclean: clean
	- $(RM) $(SHARED_L) $(DYLIB_NA)
	- $(MAKE) -C $(LIBCALLSTACK_DIR) fclean

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all install uninstall

-include $(DEPS)
