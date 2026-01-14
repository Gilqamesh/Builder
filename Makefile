CMAKE := /usr/bin/cmake
CXX   := /usr/bin/clang++
CC    := /usr/bin/clang
AR    := /usr/bin/ar

REQUIRED_TOOLS := \
	CMAKE \
	CXX \
	CC \
	AR

$(foreach required_tool,$(REQUIRED_TOOLS),\
	$(if $(shell [ -x $($(required_tool)) ] && echo yes),,\
		$(error required tool $($(required_tool)) is missing or not executable)))

REQUIRED_PARAMS := \
	SOURCE_DIR \
	LIBRARY_TYPE \
	INTERFACE_BUILD_DIR \
	INTERFACE_INSTALL_DIR \
	LIBRARIES_BUILD_DIR \
	LIBRARIES_INSTALL_DIR \
	IMPORT_BUILD_DIR \
	IMPORT_INSTALL_DIR \
	ARTIFACT_DIR \
	ARTIFACT_ALIAS_DIR

$(foreach required_param,$(REQUIRED_PARAMS),\
	$(if $(strip $($(required_param))),,\
		$(error $(required_param) is empty)))

################################################## export_interface ##################################################

.PHONY: export_interface

MODULE_NAME := builder

HEADERS := $(shell find $(SOURCE_DIR) -type f \( -name '*.h' -o -name '*.hpp' \))
INTERFACE_HEADERS := $(patsubst $(SOURCE_DIR)/%,$(INTERFACE_INSTALL_DIR)/$(MODULE_NAME)/%,$(HEADERS))

export_interface: $(INTERFACE_HEADERS)

.PHONY: FORCE
FORCE:

$(INTERFACE_INSTALL_DIR)/$(MODULE_NAME)/%: $(SOURCE_DIR)/% FORCE
	@mkdir -p $(dir $@)
	@cp $< $@

################################################## export_libraries ##################################################

.PHONY: export_libraries

CXXFLAGS := -std=c++23 -g
CFLAGS   := -g

SOURCES := \
	cmake/cmake.cpp \
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
	builder.cpp \
	module_graph.cpp \
	process/process.cpp \
	shared_library/shared_library.cpp \
	tar/tar.cpp \
	tar/external/microtar.c \
	zip/external/miniz.c \
	zip/zip.cpp

BUILDER_MODULES := cmake compiler curl filesystem gzip process shared_library tar zip

BUILDER_LIBRARY_OBJECTS := $(addprefix $(LIBRARIES_BUILD_DIR)/,$(SOURCES))
BUILDER_LIBRARY_OBJECTS := $(BUILDER_LIBRARY_OBJECTS:.cpp=.o)
BUILDER_LIBRARY_OBJECTS := $(BUILDER_LIBRARY_OBJECTS:.c=.o)

ifeq ($(LIBRARY_TYPE),static)
$(error Builder does not support static build (curl has no static library))
CURL_LIBRARY_NAME ?= libcurl.a
BUILDER_LIBRARY_NAME ?= libbuilder.a
else
CURL_LIBRARY_NAME ?= libcurl.so
BUILDER_LIBRARY_NAME ?= libbuilder.so
endif

ifeq ($(LIBRARY_TYPE),shared)
PIC := -fPIC
endif

BUILDER_LIBRARY := $(LIBRARIES_INSTALL_DIR)/$(BUILDER_LIBRARY_NAME)
CURL_LIBRARY := $(LIBRARIES_INSTALL_DIR)/$(CURL_LIBRARY_NAME)

export_libraries: export_interface
export_libraries: $(BUILDER_LIBRARY) $(CURL_LIBRARY)

$(BUILDER_LIBRARY): $(BUILDER_LIBRARY_OBJECTS) FORCE
	@mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(AR) rcs $@ $(BUILDER_LIBRARY_OBJECTS)
else
	$(CXX) -shared -o $@ $(BUILDER_LIBRARY_OBJECTS)
endif

$(CURL_LIBRARY): export_interface
	@mkdir -p $(dir $@)
ifeq ($(LIBRARY_TYPE),static)
	$(error Builder does not support static build (curl has no static library))
else
	ln -fs /usr/lib64/libcurl.so $@
endif

$(LIBRARIES_BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cpp FORCE
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PIC) -c $< -o $@

$(LIBRARIES_BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c FORCE
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PIC) -c $< -o $@

################################################## import_libraries ##################################################

.PHONY: import_libraries

CLI_SOURCES := \
	cmake/cli.cpp \
	compiler/cli.cpp \
	curl/cli.cpp \
	filesystem/cli.cpp \
	gzip/cli.cpp \
	json/cli.cpp \
	process/cli.cpp \
	shared_library/cli.cpp \
	tar/cli.cpp \
	zip/cli.cpp \
	cli.cpp

CLI_BINS := $(patsubst %.cpp,$(IMPORT_INSTALL_DIR)/%,$(CLI_SOURCES))

import_libraries: export_libraries
import_libraries: $(CLI_BINS)
import_libraries: $(ARTIFACT_ALIAS_DIR)

$(IMPORT_INSTALL_DIR)/%: $(SOURCE_DIR)/%.cpp FORCE
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $< $(shell find $(LIBRARIES_INSTALL_DIR) ! -type d)

$(ARTIFACT_ALIAS_DIR): FORCE
	ln -fsn $(ARTIFACT_DIR) $(ARTIFACT_ALIAS_DIR)
