CC ?= arm-linux-gnueabihf-gcc
CFLAGS ?= -I../ -I. -fPIC
AR ?= ar
ARFLAGS ?= rcs

CFLAGS += -O3 -march=armv8.2-a -mtune=cortex-a55 -flto

SRCS = display.c file_display.c yuv_to_rgb.c memcpy_optimized.c
#ifdef DRM
SRCS += drm_display.c
CFLAGS += -DDRM -I../include -I../include/drm -I/usr/include
#endif
ifdef WAYLAND
SRCS += wayland/wayland_display.c
endif
OBJS = $(SRCS:.c=.o)
LIB = libdisplay.so

all: $(LIB)

$(LIB): $(OBJS)
	@mkdir -p lib
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
