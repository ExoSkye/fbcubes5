.ONESHELL:

ifndef LOG_LEVEL
    LOG_LEVEL = INFO
endif

include log.mk

ifneq ($(LOG_COMMANDS),1)
    VERB := @
endif

.NOSUFFIXES:

.PHONY: all clean check_fb_name build_dir

SRC := src/cubes5.c src/fbdisplay.c
COLOUR_LIB_SRCS := src/colour.c
INCLUDES := src/fbdisplay.h src/defines.h src/colours/colour.h
INCLUDE_DIRS := -Icontrib -Isrc/colours -Isrc
CFLAGS := $(CFLAGS) -march=native -mtune=native
TARGET ?= i586-linux-elf


ifdef BUILD_DEBUG
	CFLAGS := $(CFLAGS) -g
else
	CFLAGS := $(CFLAGS) -O3 -flto
endif

ifdef BUILD_WITH_POLLY
	CFLAGS := $(CFLAGS) -target $(TARGET) -fuse-ld=lld -g -mllvm -polly
	CC := clang
endif

ifdef BUILD_PENTIUM
	CFLAGS := $(CFLAGS) -march=pentium-mmx -mtune=pentium-mmx
endif

all: Makefile build/cubes5

build_dir:
	$(VERB) mkdir -p build

build/format_test_helper: build_dir 
	$(VERB) $(call INFO_LOG,Building format_test_helper)
	$(VERB) $(CC) $(SRC) -o build/format_test_helper

build/cubes5: build_dir check_fb_name $(SRC) $(INCLUDES)
	$(VERB) $(call INFO_LOG,Building cubes5)
	$(VERB) $(CC) $(SRC) -o build/cubes5 $(INCLUDE_DIRS) $(CFLAGS)

clean:
	$(VERB) rm build/cubes5 build/format_test_helper 2> /dev/null
	$(VERB) rmdir build 2> /dev/null
