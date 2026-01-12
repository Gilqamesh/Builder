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
IMPORT_DIR   ?= $(MAKEFILE_DIR)/import

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
	filesystem/filesystem.cpp \
	gzip/gzip.cpp \
	gzip/external/adler32.c \
	gzip/external/crc32.c \
	gzip/external/deflate.c \
	gzip/external/inflate.c \
	gzip/external/inffast.c \
	gzip/external/inftrees.c \
	gzip/external/trees.c \
	gzip/external/zutil.c \
	gzip/external/gzlib.c \
	gzip/external/gzread.c \
	gzip/external/gzwrite.c \
	gzip/external/gzclose.c \
	module/module_graph.cpp \
	tar/tar.cpp \
	tar/external/microtar.c \
	zip/external/miniz.c \
	zip/zip.cpp

CLI_SRCS := \
	compiler/cli.cpp \
	curl/cli.cpp \
	filesystem/cli.cpp \
	gzip/cli.cpp \
	json/cli.cpp \
	module/cli.cpp \
	tar/cli.cpp \
	zip/cli.cpp \
	cli.cpp

CLI_BINS := $(patsubst %.cpp,$(IMPORT_DIR)/%,$(CLI_SRCS))

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC))
OBJ := $(OBJ:.cpp=.o)
OBJ := $(OBJ:.c=.o)

ifeq ($(LIBRARY_TYPE),shared)
PIC := -fPIC
endif

.PHONY: all

all: $(BUILDER_LIB)
all: $(CURL_LIB)
all: $(CLI_BINS)

.PHONY: FORCE
FORCE:

$(BUILDER_LIB): $(OBJ) FORCE
	mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(AR) rcs $@ $(OBJ)
else
	$(CXX) -shared -o $@ $(OBJ)
endif

$(CURL_LIB): FORCE
	mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(error Builder does not support static build (curl has no static library))
else
	ln -fs /usr/lib64/libcurl.so $@
endif

$(IMPORT_DIR)/%: %.cpp $(BUILDER_LIB) $(CURL_LIB) FORCE
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $< $(BUILDER_LIB) $(CURL_LIB)

$(BUILD_DIR)/%.o: %.cpp FORCE
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PIC) -c $< -o $@

$(BUILD_DIR)/%.o: %.c FORCE
	mkdir -p $(dir $@)
	$(CC)  $(CFLAGS) $(PIC) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(EXPORT_DIR) $(IMPORT_DIR)
