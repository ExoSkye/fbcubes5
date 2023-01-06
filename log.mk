COLOURS := $(shell tput colors 2>/dev/null)

ifneq ($(shell expr $(COLOURS) \>= 8),1)
	ifndef FORCE_COLOURS
        $(warning Terminal does not support 8 colours, turning off colours)
        NO_COLOUR := 1
    endif
endif

# The escape sequences here are for ANSI color codes - they are supposed to be here (example )

ifeq ($(findstring $(LOG_LEVEL),DEBUG),DEBUG)
	LOG_DEBUG := 1
	LOG_COMMANDS := 1
	LOG_VERBOSE := 1
	LOG_INFO := 1
	LOG_WARN := 1
	LOG_ERROR := 1
else ifeq ($(findstring $(LOG_LEVEL),COMMANDS),COMMANDS)
	LOG_COMMANDS := 1
	LOG_VERBOSE := 1
	LOG_INFO := 1
	LOG_WARN := 1
	LOG_ERROR := 1
else ifeq ($(findstring $(LOG_LEVEL),VERBOSE),VERBOSE)
	LOG_VERBOSE := 1
	LOG_INFO := 1
	LOG_WARN := 1
	LOG_ERROR := 1
else ifeq ($(findstring $(LOG_LEVEL),INFO),INFO)
	LOG_INFO := 1
	LOG_WARN := 1
	LOG_ERROR := 1
else ifeq ($(findstring $(LOG_LEVEL),WARN),WARN)
	LOG_WARN := 1
	LOG_ERROR := 1
else ifeq ($(findstring $(LOG_LEVEL),ERROR),ERROR)
	LOG_ERROR := 1
else
    $(error LOG_LEVEL is not set to a valid value)
endif

ifeq (1,$(NO_COLOUR))
	ERROR := [ERROR] 
	WARNING := [WARNING] 
	SUCCESS := [SUCCESS] 
	INFO := [INFO] 
	DEBUG := [DEBUG] 
	VERBOSE := [VERBOSE] 
	COMMANDS :=
	END :=
else
	ERROR := [31m
	WARNING := [33m
	SUCCESS := [32m
	INFO := [37m
	DEBUG := [36m
	VERBOSE := $(DEBUG)
	COMMANDS := $(DEBUG)
	END := [0m
endif

ERROR_LOG =     $(error $(ERROR)$(1)$(END))
WARNING_LOG = 	$(if $(LOG_WARN),$(warning $(WARNING)$(1)$(END)))
SUCCESS_LOG = 	$(if $(LOG_SUCCESS),$(info $(SUCCESS)$(1)$(END)))
INFO_LOG =      $(if $(LOG_INFO),$(info $(INFO)$(1)$(END)))
DEBUG_LOG =     $(if $(LOG_DEBUG),$(info $(DEBUG)$(1)$(END)))
VERBOSE_LOG = 	$(if $(LOG_VERBOSE),$(info $(VERBOSE)$(1)$(END)))
COMMANDS_LOG =	$(if $(LOG_COMMANDS),$(info $(COMMANDS)$(1)$(END)))

ifdef PREVIEW_LOGGING
    $(call COMMANDS_LOG,Logging commands)
    $(call VERBOSE_LOG,Logging verbosely)
    $(call DEBUG_LOG,Logging debug)
    $(call INFO_LOG,Logging info)
    $(call WARNING_LOG,Logging warning)
    $(call ERROR_LOG,Logging error)
endif