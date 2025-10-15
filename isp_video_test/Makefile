CC = arm-linux-gnueabihf-gcc
CFLAGS ?= -I./ -I./display -I./include -fPIC
LDFLAGS ?= -L./display/lib -lpthread

# DRM support - conditional linking
LDFLAGS += -L./lib -ldrm
CFLAGS += -DDRM

# WAYLAND support
ifdef WAYLAND
LDFLAGS += -lwayland-client
CFLAGS += -DWAYLAND
endif

SRCS = isp_video_test.c
OBJS = $(SRCS:.c=.o)

# Display backend objects/static lib
DISPLAY_LIB = display/libdisplay.so

TARGET = isp_video_test

all: $(TARGET)

$(TARGET): $(OBJS) $(DISPLAY_LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(DISPLAY_LIB) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(DISPLAY_LIB):
	$(MAKE) -C display

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C display clean

.PHONY: all clean
