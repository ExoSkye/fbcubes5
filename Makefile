.ONESHELL:

ifndef LOG_LEVEL
    LOG_LEVEL = INFO
endif

include log.mk

ifneq ($(LOG_COMMANDS),1)
    VERB := @
endif

.NOSUFFIXES:

.PHONY: all clean check_fb_name DEFAULT_TARGET

SRC := src/cubes5.c src/fbdisplay.c
INCLUDES := src/fbdisplay.h src/defines.h
INCLUDE_DIRS := -Icontrib

ifdef BUILD_DEBUG
	CFLAGS := -g
else
	CFLAGS := -O3 -flto
endif

ifdef BUILD_WITH_POLLY
	BUILD_PENTIUM := 1
	CFLAGS := $(CFLAGS) -target i586-linux-elf -fuse-ld=lld -g -mllvm -polly
	CC := clang
endif

ifdef BUILD_PENTIUM
	CFLAGS := $(CFLAGS) -march=pentium-mmx -mtune=pentium-mmx
endif

all: Makefile cubes5

NO_FB_NAME =

ifndef FB_NAME
	NO_FB_NAME = 1
endif

check_fb_name:
	$(if $(NO_FB_NAME),$(call ERROR_LOG,Please provide the framebuffer name))

format_test_helper: check_fb_name format_tester/format_test.c
	$(VERB) $(call INFO_LOG,Building format_test_helper)
	$(VERB) $(CC) format_tester/format_test.c -o format_test_helper -DFB_NAME="$(FB_NAME)" $(CFLAGS)

cubes5: check_fb_name format_test_helper $(SRC) $(INCLUDES)
	$(VERB) $(call INFO_LOG,Building cubes5)
	$(VERB) $(CC) $(SRC) -o cubes5 $(INCLUDE_DIRS) -Isrc -DFB_NAME="$(FB_NAME)" $(shell ./format_test_helper $(FORMAT)) $(CFLAGS)

clean:
	$(VERB) rm cubes5 format_test_helper 2> /dev/null
