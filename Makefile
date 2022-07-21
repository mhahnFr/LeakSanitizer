LIB_NAME = liblsan.a
SHARED_L = liblsan.so
DYLIB_NA = liblsan.dylib

SRC = $(shell find . -name \*.cpp)
HDR = $(shell find . -name \*.h)

OBJS = $(patsubst %.cpp, %.o, $(SRC))
DEPS = $(patsubst %.cpp, %.d, $(SRC))

LDFLAGS = 
CXXFLAGS = -std=c++17

all: $(LIB_NAME)

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
	- $(RM) $(LIB_NAME)

re: fclean
	$(MAKE) all

.PHONY: re fclean clean all

-include $(DEPS)
