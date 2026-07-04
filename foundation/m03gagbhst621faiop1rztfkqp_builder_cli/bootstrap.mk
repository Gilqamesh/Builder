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

WORKSPACE_ROOT_DIR ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
FOUNDATION_DIR := $(WORKSPACE_ROOT_DIR)/foundation
BOOTSTRAP_SEED_DIR := $(WORKSPACE_ROOT_DIR)/foundation/m03gagbhst621faiop1rztfkqp_builder_cli
BOOTSTRAP_SEED_MODULE := m03gagbhst621faiop1rztfkqp_builder_cli
CLI ?= $(WORKSPACE_ROOT_DIR)/cli
BOOTSTRAP_INCLUDE_DIR := $(WORKSPACE_ROOT_DIR)/artifacts/bootstrap/include
BOOTSTRAP_SEED_ARTIFACT_DIR := $(WORKSPACE_ROOT_DIR)/artifacts/$(BOOTSTRAP_SEED_MODULE)
BOOTSTRAP_SEED_BOOTSTRAP_DIR := $(BOOTSTRAP_SEED_ARTIFACT_DIR)/$(BOOTSTRAP_SEED_MODULE)@0
BOOTSTRAP_SEED_BUILDER_DIR := $(BOOTSTRAP_SEED_BOOTSTRAP_DIR)/builder
BOOTSTRAP_SEED_BUILDER_SO := $(BOOTSTRAP_SEED_BUILDER_DIR)/install/builder.so
BOOTSTRAP_SEED_LATEST_DIR := $(BOOTSTRAP_SEED_ARTIFACT_DIR)/latest
BOOTSTRAP_SEED_LATEST_BUILDER_LINK := $(BOOTSTRAP_SEED_LATEST_DIR)/builder
BOOTSTRAP_SEED_LATEST_BUILDER_LINK_TMP := $(BOOTSTRAP_SEED_LATEST_DIR)/builder_tmp
BOOTSTRAP_SEED_LATEST_BUILDER_SO := $(BOOTSTRAP_SEED_LATEST_BUILDER_LINK)/install/builder.so

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
	-DM03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CXX_COMPILER_PATH=\"$(CXX_COMPILER_PATH)\" \
	-DM03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_CC_COMPILER_PATH=\"$(CC_COMPILER_PATH)\" \
	-DM03GAGBHSMHR0NAW0ZPCCV4GAQ_CXX_TOOLCHAIN_AR_PATH=\"$(AR_PATH)\" \
	-DM03GAGBHSUJJF63N0W3R2W4Q6H_BUILD_PHASES_BOOTSTRAP_BUILDER_PLUGIN_PATH=\"$(BOOTSTRAP_SEED_LATEST_BUILDER_SO)\"

BOOTSTRAP_INCLUDE_FLAGS := -I$(BOOTSTRAP_INCLUDE_DIR)

BOOTSTRAP_MODULES := \
	m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain \
	m03gagbhsnusi43zogoacgj2ez_filesystem \
	m03gagbhsp2drqq3gkop8pzfrm_workspace_graph \
	m03gagbhsqfsqblhwvelrou7nc_json \
	m03gagbhst621faiop1rztfkqp_builder_cli \
	m03gagbhsujjf63n0w3r2w4q6h_build_phases \
	m03gagbhsvr0m5w15urj0o291m_process \
	m03gagbhsyhlx2pk5sdabbr1sx_signal_handler \
	m03gagbhsx4j5z28bqkac3dhhh_shared_library \
	m03gagbht2l61mj6qitacwbmea_byte_stream \
	m03gagbhtft23yhjwpp881tfmc_uuid

BOOTSTRAP_INCLUDE_LINKS := $(addprefix $(BOOTSTRAP_INCLUDE_DIR)/,$(BOOTSTRAP_MODULES))

SRC := \
	$(FOUNDATION_DIR)/m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.cpp \
	$(FOUNDATION_DIR)/m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.cpp \
	$(FOUNDATION_DIR)/m03gagbhsvr0m5w15urj0o291m_process/process.cpp \
	$(FOUNDATION_DIR)/m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.cpp \
	$(FOUNDATION_DIR)/m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.cpp \
	$(FOUNDATION_DIR)/m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.cpp \
	$(FOUNDATION_DIR)/m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.cpp \
	$(FOUNDATION_DIR)/m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.cpp \
	$(FOUNDATION_DIR)/m03gagbhtft23yhjwpp881tfmc_uuid/uuid.cpp \
	$(FOUNDATION_DIR)/m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.cpp \
	$(BOOTSTRAP_SEED_DIR)/cli.cpp

BOOTSTRAP_SEED_BUILDER_SRC := \
	$(FOUNDATION_DIR)/m03gagbhsnusi43zogoacgj2ez_filesystem/filesystem.cpp \
	$(FOUNDATION_DIR)/m03gagbhsyhlx2pk5sdabbr1sx_signal_handler/signal_handler.cpp \
	$(FOUNDATION_DIR)/m03gagbhsvr0m5w15urj0o291m_process/process.cpp \
	$(FOUNDATION_DIR)/m03gagbhsmhr0naw0zpccv4gaq_cxx_toolchain/cxx_toolchain.cpp \
	$(FOUNDATION_DIR)/m03gagbhsx4j5z28bqkac3dhhh_shared_library/shared_library.cpp \
	$(FOUNDATION_DIR)/m03gagbhsp2drqq3gkop8pzfrm_workspace_graph/workspace_graph.cpp \
	$(FOUNDATION_DIR)/m03gagbhsujjf63n0w3r2w4q6h_build_phases/build_phases.cpp \
	$(FOUNDATION_DIR)/m03gagbht2l61mj6qitacwbmea_byte_stream/byte_stream.cpp \
	$(FOUNDATION_DIR)/m03gagbhtft23yhjwpp881tfmc_uuid/uuid.cpp \
	$(FOUNDATION_DIR)/m03gagbhst621faiop1rztfkqp_builder_cli/builder_cli.cpp \
	$(BOOTSTRAP_SEED_DIR)/builder.cpp

.PHONY: bootstrap
bootstrap: $(CLI) $(BOOTSTRAP_SEED_LATEST_BUILDER_SO)

$(BOOTSTRAP_INCLUDE_DIR)/%:
	@$(MKDIR) -p $(dir $@)
	@$(RM) -rf "$@"
	$(LN) -s "$(FOUNDATION_DIR)/$*" "$@"

$(CLI): $(SRC) $(BOOTSTRAP_INCLUDE_LINKS)
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(BOOTSTRAP_INCLUDE_FLAGS) $(SRC) $(LDFLAGS) -o $@

$(BOOTSTRAP_SEED_BUILDER_SO): $(BOOTSTRAP_SEED_BUILDER_SRC) $(BOOTSTRAP_INCLUDE_LINKS)
	@$(MKDIR) -p $(dir $@)
	$(CXX) $(CXXFLAGS) -fPIC $(DEFINES) $(BOOTSTRAP_INCLUDE_FLAGS) -shared $(BOOTSTRAP_SEED_BUILDER_SRC) $(LDFLAGS) -o $@

$(BOOTSTRAP_SEED_LATEST_BUILDER_SO): $(BOOTSTRAP_SEED_BUILDER_SO)
	@$(MKDIR) -p $(BOOTSTRAP_SEED_LATEST_DIR)
	@$(RM) -f "$(BOOTSTRAP_SEED_LATEST_BUILDER_LINK_TMP)"
	$(LN) -s ../$(BOOTSTRAP_SEED_MODULE)@0/builder "$(BOOTSTRAP_SEED_LATEST_BUILDER_LINK_TMP)"
	$(MV) -Tf "$(BOOTSTRAP_SEED_LATEST_BUILDER_LINK_TMP)" "$(BOOTSTRAP_SEED_LATEST_BUILDER_LINK)"
