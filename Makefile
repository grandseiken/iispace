.SUFFIXES:

# Default target.
.PHONY: default
default: all
# Dependencies.
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
OBJECT_FILE_PREREQS=\
  $(DEPENDENCIES)/ois.build \
  $(DEPENDENCIES)/sfml.build \
  $(DEPENDENCIES)/zlib.build

# Compilers and interpreters.
export SHELL=/bin/sh
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

H_FILE=$(wildcard $(SRCDIR)/*.h)
CPP_FILES=$(filter-out %/main.cpp,$(wildcard $(SRCDIR)/*.cpp))
SOURCE_FILES=$(CPP_FILES) $(CC_GENERATED_FILES)
OBJECT_FILES=$(addprefix $(OUTDIR_TMP)/,$(addsuffix .o,$(SOURCE_FILES)))
INCLUDE_FILES=$(wildcard $(INCLUDE)/*/*.h)
UTIL_CPP_FILES=$(wildcard $(SRCDIR)/utils/*.cpp)

DEP_FILES=\
  $(addprefix $(OUTDIR_TMP)/,$(addsuffix .deps,\
  $(SOURCE_FILES) $(UTIL_CPP_FILES)))

MISC_FILES=Makefile dependencies/Makefile
ALL_FILES=$(CPP_FILES) $(UTIL_CPP_FILES) $(H_FILES) $(MISC_FILES)

# Master targets.
.PHONY: all
all: $(IISPACE_BINARY) tools
.PHONY: tools
tools: $(BINARIES)
.PHONY: add
add:
	git add $(ALL_FILES)
.PHONY: todo
todo:
	@grep --color -n "\bT[O]D[O]\b" $(ALL_FILES)
.PHONY: wc
wc:
	wc $(ALL_FILES)
.PHONY: clean
clean:
	rm -rf $(OUTDIR) $(GENDIR)
.PHONY: clean_all
clean_all: clean clean_dependencies

# Dependency generation. Each source file generates a corresponding .deps file
# (a Makefile containing a .build target), which is then included. Inclusion
# forces regeneration via the rules provided. Deps rule depends on same .build
# target it generates. When the specific .build target doesn't exist, the
# default causes everything to be generated.
.SECONDEXPANSION:
$(DEP_FILES): $(OUTDIR_TMP)/%.deps: \
  $(OUTDIR_TMP)/%.build $(OUTDIR_TMP)/%.mkdir $(CC_GENERATED_FILES) \
  $$(subst \
  $$(OUTDIR_TMP)/,,$$($$(subst .,_,$$(subst /,_,$$(subst \
  $$(OUTDIR_TMP)/,,./$$(@:.deps=))))_LINK:.o=))
	SOURCE_FILE=$(subst $(OUTDIR_TMP)/,,./$(@:.deps=)); \
	    echo Generating dependencies for $$SOURCE_FILE; \
	    $(CXX) $(CFLAGS) -o $@ -MM $$SOURCE_FILE && \
	    sed -i -e 's/.*\.o:/$(subst /,\/,$<)::/g' $@
	echo "	@touch" $< >> $@

.PRECIOUS: $(OUTDIR_TMP)/%.build
$(OUTDIR_TMP)/%.build: \
  ./% $(OUTDIR_TMP)/%.mkdir
	touch $@

ifneq ('$(MAKECMDGOALS)', 'add')
ifneq ('$(MAKECMDGOALS)', 'todo')
ifneq ('$(MAKECMDGOALS)', 'wc')
ifneq ('$(MAKECMDGOALS)', 'clean')
ifneq ('$(MAKECMDGOALS)', 'clean_all')
-include $(DEP_FILES)
endif
endif
endif
endif
endif

# Binary linking.
$(BINARIES): $(OUTDIR_BIN)/%: \
  $(OUTDIR_TMP)/$(SRCDIR)/%.cpp.o $(OBJECT_FILES) $(OUTDIR_BIN)/%.mkdir
	@echo Linking ./$@
	$(CXX) -o ./$@ $< $(OBJECT_FILES) $(LFLAGS)

# Object files. References dependencies that must be built before their header
# files are available.
$(OUTDIR_TMP)/%.o: \
  $(OUTDIR_TMP)/%.build $(OUTDIR_TMP)/%.mkdir $(OBJECT_FILE_PREREQS)
	SOURCE_FILE=$(subst $(OUTDIR_TMP)/,,./$(<:.build=)); \
	    echo Compiling $$SOURCE_FILE; \
	    $(CXX) -c $(CFLAGS) $(if $(findstring /./gen/,$@),,$(WFLAGS)) \
	    -o $@ $$SOURCE_FILE

# Proto files.
.PRECIOUS: $(CC_GENERATED_FILES)
$(GENDIR)/%.pb.h: \
  $(GENDIR)/%.pb.cc
	touch $@ $<
$(GENDIR)/%.pb.cc: \
  $(SRCDIR)/%.proto $(GENDIR)/%.mkdir $(DEPENDENCIES)/protobuf.build
	@echo Compiling ./$<
	$(PROTOC) --proto_path=$(SRCDIR) --cpp_out=$(GENDIR) ./$<

# Ensure a directory exists.
.PRECIOUS: ./%.mkdir
./%.mkdir:
	mkdir -p $(dir $@)
	touch $@
