.PHONY: first
first: all
include dependencies/Makefile

# Debug options.
ifeq ($(DBG), 1)
OUTDIR=./Debug
WFLAGS=-Werror -Wall -Wextra -Wpedantic
CFLAGS_CONFIG=-Og -g -ggdb -DDEBUG
else
OUTDIR=./Release
CFLAGS_CONFIG=-O3
endif

# Directories.
OUTDIR_BIN=$(OUTDIR)
OUTDIR_TMP=$(OUTDIR)/build
SRCDIR=./src
GENDIR=./gen
IISPACE_BINARY=$(OUTDIR_BIN)/main
GAMEPAD_BINARY=$(OUTDIR_BIN)/util/gamepad
RECRYPT_BINARY=$(OUTDIR_BIN)/util/recrypt
BINARIES=$(IISPACE_BINARY) $(GAMEPAD_BINARY) $(RECRYPT_BINARY)

# Libraries.
CC_OBJECT_FILE_PREREQS=\
  $(DEPENDENCIES)/ois.build \
  $(DEPENDENCIES)/sfml.build \
  $(DEPENDENCIES)/zlib.build

# Compilers and interpreters.
export PROTOC=$(PROTOBUF_DIR)/src/protoc

CFLAGS_11=-std=c++11
CFLAGS=\
  $(CFLAGS_EXTRA) $(CFLAGS_11) $(CFLAGS_CONFIG) -DPLATFORM_LINUX \
  -isystem $(OIS_DIR)/includes -isystem $(SFML_DIR)/include -isystem $(ZLIB_DIR)
LFLAGS=\
  $(LFLAGSEXTRA) \
  $(OIS_DIR)/src/.libs/libOIS.a \
  $(PROTOBUF_DIR)/src/.libs/libprotobuf.a \
  $(SFML_DIR)/lib/libsfml-audio-s.a \
  $(SFML_DIR)/lib/libsfml-graphics-s.a \
  $(SFML_DIR)/lib/libsfml-window-s.a \
  $(SFML_DIR)/lib/libsfml-system-s.a \
  $(ZLIB_DIR)/libz.a \
  -lX11 -lGL -lopenal -lsndfile -lXrandr

# File listings.
PROTO_FILES=$(wildcard $(SRCDIR)/*.proto)
PROTO_OUTPUTS=$(subst $(SRCDIR)/,$(GENDIR)/,$(PROTO_FILES:.proto=.pb.cc))
CC_GENERATED_FILES=$(PROTO_OUTPUTS)

H_FILES=$(wildcard $(SRCDIR)/*.h)
CPP_FILES=$(wildcard $(SRCDIR)/*.cpp)
UTIL_CPP_FILES=$(wildcard $(SRCDIR)/util/*.cpp)
CC_SOURCE_FILES=$(CPP_FILES) $(UTIL_CPP_FILES)
FILTERED_OBJECT_FILES=$(addprefix $(OUTDIR_TMP)/,$(addsuffix .o,$(filter-out $(SRCDIR)/main.cpp, $(CPP_FILES) $(CC_GENERATED_FILES))))
INCLUDE_FILES=$(wildcard $(INCLUDE)/*/*.h)

MISC_FILES=Makefile dependencies/Makefile
ALL_FILES=$(CPP_FILES) $(UTIL_CPP_FILES) $(H_FILES) $(MISC_FILES)

DISABLE_CC_DEPENDENCY_ANALYSIS=true
ifneq ('$(MAKECMDGOALS)', 'add')
ifneq ('$(MAKECMDGOALS)', 'todo')
ifneq ('$(MAKECMDGOALS)', 'wc')
ifneq ('$(MAKECMDGOALS)', 'clean')
ifneq ('$(MAKECMDGOALS)', 'clean_all')
DISABLE_CC_DEPENDENCY_ANALYSIS=false
endif
endif
endif
endif
endif
include dependencies/makelib/Makefile

# Master targets.
.PHONY: all
all: $(IISPACE_BINARY) tools
.PHONY: tools
tools: $(BINARIES)
.PHONY: clean
clean:
	rm -rf $(OUTDIR) $(GENDIR)
.PHONY: clean_all
clean_all: clean clean_dependencies

# Binary linking.
$(BINARIES): $(OUTDIR_BIN)/%: \
  $(OUTDIR_TMP)/$(SRCDIR)/%.cpp.o $(FILTERED_OBJECT_FILES)
	$(MKDIR)
	@echo Linking ./$@
	$(CXX) -o ./$@ $< $(FILTERED_OBJECT_FILES) $(LFLAGS)

# Proto files.
$(GENDIR)/%.pb.h: \
  $(GENDIR)/%.pb.cc
	touch $@ $<
$(GENDIR)/%.pb.cc: \
  $(SRCDIR)/%.proto $(DEPENDENCIES)/protobuf.build
	$(MKDIR)
	@echo Compiling ./$<
	$(PROTOC) --proto_path=$(SRCDIR) --cpp_out=$(GENDIR) ./$<
