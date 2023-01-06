.ONESHELL:

ifndef LOG_LEVEL
    LOG_LEVEL = INFO
endif

include log.mk

ifeq ($(LOG_COMMANDS),1)
	VERB := "@"
endif

.NOSUFFIXES:

.PHONY: all

SRC := src/cubes5.c src/fbdisplay.c
INCLUDES := src/fbdisplay.h src/defines.h
INCLUDE_DIRS := -Icontrib

cubes5: $(SRC) $(INCLUDES)
	$(call INFO_LOG,Building cubes5)
	@$(CC) $(SRC) -o cubes5 $(INCLUDE_DIRS) -Isrc

all: cubes5