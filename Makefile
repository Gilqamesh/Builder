CXX := clang++
CC  := clang
AR  := ar

MAKEFILE_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
INCLUDE_DIR  := $(abspath $(MAKEFILE_DIR)/..)

CXXFLAGS := -std=c++23 -g -I$(INCLUDE_DIR)
CFLAGS   := -g -I$(INCLUDE_DIR)

LIBRARY_TYPE ?= static
INSTALL_ROOT ?= $(MAKEFILE_DIR)

ifeq ($(LIBRARY_TYPE),static)
LIBRARY_NAME ?= libbuilder.a
else
LIBRARY_NAME ?= libbuilder.so
endif

INSTALL_DIR := $(INSTALL_ROOT)/$(LIBRARY_TYPE)
BUILDDIR := $(INSTALL_DIR)/cache
LIB      := $(INSTALL_DIR)/install/$(LIBRARY_NAME)

SRC := \
	zip/external/miniz.c \
	compiler/cpp_compiler.cpp \
	curl/curl.cpp \
	find/find.cpp \
	zip/zip.cpp \
	builder_api.cpp \
	builder_ctx.cpp

OBJ := $(addprefix $(BUILDDIR)/,$(SRC))
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

$(BUILDDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PIC) -c $< -o $@

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC)  $(CFLAGS)   $(PIC) -c $< -o $@

CLI := cli
CLI_SRC := cli.cpp

.PHONY: cli

cli: $(LIB)
	$(CXX) $(CXXFLAGS) -o cli $(CLI_SRC) $(LIB) -lcurl

clean:
	rm -rf $(INSTALL_DIR) $(CLI)
