# Project Name
TARGET = bypass

# Sources
CPP_SOURCES = bypass.cpp octave.cpp yin.cpp

# Library Locations
DAISYSP_DIR ?= ./DaisySP
LIBDAISY_DIR ?= ./libDaisy

OPT = -O0

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

