CC := gcc
CXX := g++
NATURE_ROOT := ../nature-os
CFLAGS := -O3 -Wall -Wextra -I$(NATURE_ROOT)/include -I$(NATURE_ROOT)/compositor/src -D_POSIX_C_SOURCE=200809L
CXXFLAGS := -O3 -std=c++20 -Wall -Wextra -I$(NATURE_ROOT)/include -I$(NATURE_ROOT)/compositor/src -D_POSIX_C_SOURCE=200809L
LDFLAGS := -ldl -lpthread -lm

TARGET := build/nature-previewer

C_SRCS := \
	$(NATURE_ROOT)/compositor/src/gfx_cairo.c \
	$(NATURE_ROOT)/compositor/src/render_scheduler.c \
	$(NATURE_ROOT)/compositor/src/main.c \
	$(NATURE_ROOT)/shell/src/theme.c \
	$(NATURE_ROOT)/shell/src/icons.c \
	$(NATURE_ROOT)/shell/src/persistence.c \
	$(NATURE_ROOT)/shell/src/wm.c \
	$(NATURE_ROOT)/shell/src/overlays.c \
	$(NATURE_ROOT)/shell/src/shell.c \
	$(NATURE_ROOT)/apps/nature-files/main.c \
	$(NATURE_ROOT)/apps/nature-terminal/main.c \
	$(NATURE_ROOT)/apps/nature-settings/main.c \
	$(NATURE_ROOT)/apps/nature-media/main.c

CXX_SRCS := \
	$(NATURE_ROOT)/apps/nature-monitor/main.cpp \
	$(NATURE_ROOT)/apps/nature-store/main.cpp \
	$(NATURE_ROOT)/apps/nature-browser/main.cpp \
	src/fb_transport.cpp

C_OBJS := $(patsubst %.c,build/obj/%.o,$(subst ../,up_,$(C_SRCS)))
CXX_OBJS := $(patsubst %.cpp,build/obj/%.o,$(subst ../,up_,$(CXX_SRCS)))

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CXX_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build/obj/up_%.o: ../%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/obj/up_%.o: ../%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

.PHONY: all clean
