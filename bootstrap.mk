ifeq ($(origin CXX),default)
CXX := /usr/bin/clang++
endif

ifeq ($(origin CC),default)
CC := /usr/bin/clang
endif

ifeq ($(origin AR),default)
AR := /usr/bin/ar
endif

CPP_COMPILER_PATH ?= $(CXX)
C_COMPILER_PATH   ?= $(CC)
AR_PATH           ?= $(AR)

WORKSPACE_ECOSYSTEM_DIR ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
KERNEL_DIR := $(WORKSPACE_ECOSYSTEM_DIR)/kernel/cpp_builder
CLI ?= $(WORKSPACE_ECOSYSTEM_DIR)/cli

REQUIRED_TOOLS := \
	CXX \
	CC \
	AR

$(foreach required_tool,$(REQUIRED_TOOLS),\
	$(if $(shell [ -x "$($(required_tool))" ] && echo yes),,\
		$(error required tool $($(required_tool)) is missing or not executable)))

CXXFLAGS ?= -g -std=c++23
LDFLAGS  ?= -ldl

DEFINES := \
	-DKERNEL_CPP_BUILDER_CPP_COMPILER_PATH=\"$(CPP_COMPILER_PATH)\" \
	-DKERNEL_CPP_BUILDER_C_COMPILER_PATH=\"$(C_COMPILER_PATH)\" \
	-DKERNEL_CPP_BUILDER_AR_PATH=\"$(AR_PATH)\"

SRC := \
	$(KERNEL_DIR)/filesystem.cpp \
	$(KERNEL_DIR)/process.cpp \
	$(KERNEL_DIR)/compiler.cpp \
	$(KERNEL_DIR)/shared_library.cpp \
	$(KERNEL_DIR)/graph.cpp \
	$(KERNEL_DIR)/module_builder.cpp \
	$(KERNEL_DIR)/cli.cpp

.PHONY: bootstrap
bootstrap: $(CLI)

$(CLI): $(SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(SRC) $(LDFLAGS) -o $@
