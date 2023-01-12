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
INCLUDES := src/fbdisplay.h src/defines.h
INCLUDE_DIRS := -Icontrib
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

NO_FB_NAME =

ifndef FB_NAME
	NO_FB_NAME = 1
endif

check_fb_name:
	$(if $(NO_FB_NAME),$(call ERROR_LOG,Please provide the framebuffer name))

build_dir:
	$(VERB) mkdir -p build

build/format_test_helper: build_dir check_fb_name format_tester/format_test.c
	$(VERB) $(call INFO_LOG,Building format_test_helper)
	$(VERB) $(CC) format_tester/format_test.c -o build/format_test_helper -DFB_NAME="$(FB_NAME)" $(CFLAGS)

build/cubes5: build_dir check_fb_name build/format_test_helper $(SRC) $(INCLUDES)
	$(VERB) $(call INFO_LOG,Building cubes5)

	$(if $(findstring DR,$(shell build/format_test_helper $(FORMAT))),$(VERB) $(CC) $(SRC) -o build/cubes5 $(INCLUDE_DIRS) -Isrc -DFB_NAME="$(FB_NAME)" $(CFLAGS) $(shell build/format_test_helper $(FORMAT)), \
	$(file > build/format_test_helper.log,$(shell build/format_test_helper $(FORMAT))) \
	$(call ERROR_LOG,format_test_helper didnt return a valid format (see build/format_test_helper.log)))

clean:
	$(VERB) rm build/cubes5 build/format_test_helper 2> /dev/null
	$(VERB) rmdir build 2> /dev/null
