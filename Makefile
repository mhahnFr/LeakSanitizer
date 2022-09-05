#
# LeakSanitizer - A small library showing informations about lost memory.
#
# Copyright (C) 2022  mhahnFr
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

LIB_NAME = $(CORE_NAME).a
SHARED_L = $(CORE_NAME).so
DYLIB_NA = $(CORE_NAME).dylib

SRC = $(shell find . -name \*.cpp)
HDR = $(shell find . -name \*.h)

OBJS = $(patsubst %.cpp, %.o, $(SRC))
DEPS = $(patsubst %.cpp, %.d, $(SRC))

LDFLAGS = -ldl
CXXFLAGS = -std=c++17 -Wall -pedantic -fPIC -O0

NO_LICENSE = true
ifeq ($(NO_LICENSE),true)
	CXXFLAGS += -DNO_LICENSE
endif

CPP_TRACK = false
ifeq ($(CPP_TRACK),true)
	CXXFLAGS += -DCPP_TRACK
endif

NAME = $(SHARED_L)
ifeq ($(shell uname -s), Linux)
	NAME = $(LIB_NAME)
else ifeq ($(shell uname -s), Darwin)
	NAME = $(LIB_NAME)
endif

VERSION = "clean build"
ifneq ($(shell git tag | tail -n 1),)
	VERSION = $(shell git tag | tail -n 1)
endif

default: $(NAME)

all: $(LIB_NAME) $(SHARED_L) $(DYLIB_NA)

$(SHARED_L): $(OBJS)
	$(CXX) -shared -fPIC $(LDFLAGS) -o $(SHARED_L) $(OBJS)

$(DYLIB_NA): $(OBJS)
	$(CXX) -dynamiclib $(LDFLAGS) -o $(DYLIB_NA) $(OBJS)

$(LIB_NAME): $(OBJS)
	$(AR) -crs $(LIB_NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -DVERSION=\"$(VERSION)\" -MMD -MP -c -o $@ $<

clean:
	- $(RM) $(OBJS) $(DEPS)

fclean: clean
	- $(RM) $(LIB_NAME) $(SHARED_L) $(DYLIB_NA)

re: fclean
	$(MAKE) default

.PHONY: re fclean clean all

-include $(DEPS)
