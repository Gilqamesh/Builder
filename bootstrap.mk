CXX = /usr/bin/clang++
CC = /usr/bin/clang
AR = /usr/bin/ar
LN = /usr/bin/ln
MKDIR = /usr/bin/mkdir
MV = /usr/bin/mv
RM = /usr/bin/rm

CXX_COMPILER_PATH = $(CXX)
CC_COMPILER_PATH = $(CC)
AR_PATH = $(AR)

WORKSPACE_ECOSYSTEM_DIR ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
KERNEL_DIR := $(WORKSPACE_ECOSYSTEM_DIR)/kernel/cpp_builder
CLI ?= $(WORKSPACE_ECOSYSTEM_DIR)/cli
KERNEL_ARTIFACT_DIR := $(WORKSPACE_ECOSYSTEM_DIR)/artifacts/kernel/cpp_builder
KERNEL_BOOTSTRAP_DIR := $(KERNEL_ARTIFACT_DIR)/cpp_builder@0
KERNEL_BUILDER_DIR := $(KERNEL_BOOTSTRAP_DIR)/builder
KERNEL_BUILDER_SO := $(KERNEL_BUILDER_DIR)/install/builder.so
KERNEL_LATEST_DIR := $(KERNEL_ARTIFACT_DIR)/latest
KERNEL_LATEST_BUILDER_LINK := $(KERNEL_LATEST_DIR)/builder
KERNEL_LATEST_BUILDER_LINK_TMP := $(KERNEL_LATEST_DIR)/builder_tmp
KERNEL_LATEST_BUILDER_SO := $(KERNEL_LATEST_BUILDER_LINK)/install/builder.so

REQUIRED_TOOLS := \
	CXX \
	CC \
	AR \
	LN \
	MKDIR \
	MV \
	RM

$(foreach required_tool,$(REQUIRED_TOOLS),\
	$(if $(shell [ -x "$($(required_tool))" ] && echo yes),,\
		$(error required tool $($(required_tool)) is missing or not executable)))

CXXFLAGS ?= -g -std=c++23
LDFLAGS  ?= -ldl

DEFINES := \
	-DKERNEL_CPP_BUILDER_CXX_COMPILER_PATH=\"$(CXX_COMPILER_PATH)\" \
	-DKERNEL_CPP_BUILDER_CC_COMPILER_PATH=\"$(CC_COMPILER_PATH)\" \
	-DKERNEL_CPP_BUILDER_AR_PATH=\"$(AR_PATH)\"

SRC := \
	$(KERNEL_DIR)/filesystem.cpp \
	$(KERNEL_DIR)/process.cpp \
	$(KERNEL_DIR)/compiler.cpp \
	$(KERNEL_DIR)/shared_library.cpp \
	$(KERNEL_DIR)/graph.cpp \
	$(KERNEL_DIR)/phase.cpp \
	$(KERNEL_DIR)/module_builder.cpp \
	$(KERNEL_DIR)/cli.cpp

KERNEL_BUILDER_SRC := \
	$(KERNEL_DIR)/filesystem.cpp \
	$(KERNEL_DIR)/process.cpp \
	$(KERNEL_DIR)/compiler.cpp \
	$(KERNEL_DIR)/shared_library.cpp \
	$(KERNEL_DIR)/graph.cpp \
	$(KERNEL_DIR)/phase.cpp \
	$(KERNEL_DIR)/module_builder.cpp \
	$(KERNEL_DIR)/builder.cpp

.PHONY: bootstrap
bootstrap: $(CLI) $(KERNEL_LATEST_BUILDER_SO)

$(CLI): $(SRC)
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(SRC) $(LDFLAGS) -o $@

$(KERNEL_BUILDER_SO): $(KERNEL_BUILDER_SRC)
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -fPIC $(DEFINES) -shared $(KERNEL_BUILDER_SRC) $(LDFLAGS) -o $@

$(KERNEL_LATEST_BUILDER_SO): $(KERNEL_BUILDER_SO)
	@$(MKDIR) -p $(KERNEL_LATEST_DIR)
	@$(RM) -f "$(KERNEL_LATEST_BUILDER_LINK_TMP)"
	$(LN) -s ../cpp_builder@0/builder "$(KERNEL_LATEST_BUILDER_LINK_TMP)"
	$(MV) -Tf "$(KERNEL_LATEST_BUILDER_LINK_TMP)" "$(KERNEL_LATEST_BUILDER_LINK)"
