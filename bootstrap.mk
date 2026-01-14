MODULES_DIR ?= $(error MODULES_DIR is required)
ARTIFACTS_DIR ?= $(error ARTIFACTS_DIR is required)

TARGET_MODULE := builder

SRC_REL := \
	compiler/cpp_compiler.cpp \
	filesystem/filesystem.cpp \
	process/process.cpp \
	shared_library/shared_library.cpp \
	builder.cpp \
	cli.cpp \
	module_graph.cpp
SRC := $(patsubst %,$(MODULES_DIR)/$(TARGET_MODULE)/%,$(SRC_REL))

INCLUDES := -I$(MODULES_DIR)

CLI := $(ARTIFACTS_DIR)/$(TARGET_MODULE)/.bootstrap/cli

.PHONY: bootstrap
bootstrap:
	@mkdir -p $(dir $(CLI))
	clang++ -g -std=c++23 $(INCLUDES) $(SRC) -o $(CLI)
	$(CLI) $(MODULES_DIR) $(TARGET_MODULE) $(ARTIFACTS_DIR)
