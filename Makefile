#
# LeakSanitizer - A small library showing informations about lost memory.
#
# Copyright (C) 2022  mhahnFr
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

CORE_NAME = liblsan

LIB_NAME = $(CORE_NAME).a
SHARED_L = $(CORE_NAME).so
DYLIB_NA = $(CORE_NAME).dylib

SRC = $(shell find . -name \*.cpp)
HDR = $(shell find . -name \*.h)

OBJS = $(patsubst %.cpp, %.o, $(SRC))
DEPS = $(patsubst %.cpp, %.d, $(SRC))

LDFLAGS = 
CXXFLAGS = -std=c++17

all: $(LIB_NAME) $(SHARED_L) $(DYLIB_NA)

$(SHARED_L): $(OBJS)
	$(CXX) -shared -fPIC -o $(SHARED_L) $(OBJS)

$(DYLIB_NA): $(OBJS)
	$(CXX) -dynamiclib -o $(DYLIB_NA) $(OBJS)

$(LIB_NAME): $(OBJS)
	$(AR) -crs $(LIB_NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	- $(RM) $(OBJS) $(DEPS)

fclean: clean
	- $(RM) $(LIB_NAME) $(SHARED_L) $(DYLIB_NA)

re: fclean
	$(MAKE) all

.PHONY: re fclean clean all

-include $(DEPS)
