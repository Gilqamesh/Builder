MODULES_DIR ?= $(error MODULES_DIR is required)
ARTIFACTS_DIR ?= $(error ARTIFACTS_DIR is required)

TARGET_MODULE := builder

SRC_REL := \
	cpp_compiler.cpp \
	filesystem.cpp \
	process.cpp \
	shared_library.cpp \
	module_builder.cpp \
	module.cpp \
	cli.cpp
SRC := $(patsubst %,$(MODULES_DIR)/$(TARGET_MODULE)/%,$(SRC_REL))

CLI := $(ARTIFACTS_DIR)/$(TARGET_MODULE)/.bootstrap/cli

.PHONY: bootstrap
bootstrap:
	@mkdir -p $(dir $(CLI))
	clang++ -g -std=c++23 $(SRC) -o $(CLI)
	$(CLI) $(MODULES_DIR) $(TARGET_MODULE) $(ARTIFACTS_DIR)
