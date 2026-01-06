CXX := clang++
CC  := clang
AR  := ar

MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INCLUDE_DIR  := $(abspath $(MAKEFILE_DIR)/../..)

CXXFLAGS := -std=c++23 -g -I$(INCLUDE_DIR)
CFLAGS   := -g -I$(INCLUDE_DIR)

LIBRARY_TYPE ?= shared
BUILD_DIR    ?= $(MAKEFILE_DIR)/build/$(LIBRARY_TYPE)
EXPORT_DIR   ?= $(MAKEFILE_DIR)/export/$(LIBRARY_TYPE)

ifeq ($(LIBRARY_TYPE),static)
$(error Builder does not support static build (curl has no static library))
CURL_LIB_NAME ?= libcurl.a
BUILDER_LIB_NAME ?= libbuilder.a
else
CURL_LIB_NAME ?= libcurl.so
BUILDER_LIB_NAME ?= libbuilder.so
endif

BUILDER_LIB := $(EXPORT_DIR)/$(BUILDER_LIB_NAME)
CURL_LIB := $(EXPORT_DIR)/$(CURL_LIB_NAME)

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

all: $(BUILDER_LIB)
all: $(CURL_LIB)

$(BUILDER_LIB): $(OBJ)
	mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(AR) rcs $@ $(OBJ)
else
	$(CXX) -shared -o $@ $(OBJ)
endif

$(CURL_LIB):
	mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(error Builder does not support static build (curl has no static library))
else
	ln -s /usr/lib64/libcurl.so $@
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

cli: $(BUILDER_LIB) $(CURL_LIB)
	$(CXX) $(CXXFLAGS) -o cli $(CLI_SRC) $(BUILDER_LIB) $(CURL_LIB)
	rm -rf $(BUILDER_LIB) $(CURL_LIB)

clean:
	rm -rf $(BUILD_DIR) $(EXPORT_DIR) $(CLI)
