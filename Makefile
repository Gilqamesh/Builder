CXX := clang++
CC  := clang
AR  := ar

MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INCLUDE_DIR  := $(abspath $(MAKEFILE_DIR)/..)

CXXFLAGS := -std=c++23 -g -I$(INCLUDE_DIR)
CFLAGS   := -g -I$(INCLUDE_DIR)

LIBRARY_TYPE ?= static
BUILD_DIR    ?= $(MAKEFILE_DIR)/build/$(LIBRARY_TYPE)
EXPORT_DIR   ?= $(MAKEFILE_DIR)/export/$(LIBRARY_TYPE)

ifeq ($(LIBRARY_TYPE),static)
LIBRARY_NAME ?= libbuilder.a
else
LIBRARY_NAME ?= libbuilder.so
endif

LIB := $(EXPORT_DIR)/$(LIBRARY_NAME)

SRC := \
	compiler/cpp_compiler.cpp \
	curl/curl.cpp \
	find/find.cpp \
	module/module_graph.cpp \
	zip/external/miniz.c \
	zip/zip.cpp

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC))
OBJ := $(OBJ:.cpp=.o)
OBJ := $(OBJ:.c=.o)

ifeq ($(LIBRARY_TYPE),shared)
PIC := -fPIC
endif

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(AR) rcs $@ $(OBJ)
else
	$(CXX) -shared -o $@ $(OBJ)
endif

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PIC) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC)  $(CFLAGS) $(PIC) -c $< -o $@

CLI := cli
CLI_SRC := cli.cpp

.PHONY: cli

cli: $(LIB)
	$(CXX) $(CXXFLAGS) -o cli $(CLI_SRC) $(LIB) -lcurl

clean:
	rm -rf $(BUILD_DIR) $(EXPORT_DIR) $(CLI)
