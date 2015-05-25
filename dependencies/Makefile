# Dependency directories.
DEPENDENCIES=./dependencies
OIS_DIR=$(DEPENDENCIES)/ois_1_3
PROTOBUF_DIR=$(DEPENDENCIES)/protobuf_2_5_0
SFML_DIR=$(DEPENDENCIES)/sfml_1_6
ZLIB_DIR=$(DEPENDENCIES)/zlib_1_2_8

# Build OIS.
$(DEPENDENCIES)/ois.build:
	@echo Building OIS
	cd $(OIS_DIR) && ./bootstrap
	cd $(OIS_DIR) && ./configure
	cd $(OIS_DIR) && $(MAKE)
	touch $@

# Build Protobuf.
$(DEPENDENCIES)/protobuf.build:
	@echo Building Protobuf
	cd $(PROTOBUF_DIR) && ./configure
	cd $(PROTOBUF_DIR) && $(MAKE)
	touch $@

# Build SFML.
$(DEPENDENCIES)/sfml.build:
	@echo Building SFML
	cd $(SFML_DIR) && $(MAKE) STATIC=yes
	touch $@

# Build Zlib.
$(DEPENDENCIES)/zlib.build:
	@echo Building Zlib
	cd $(ZLIB_DIR) && ./configure
	cd $(ZLIB_DIR) && $(MAKE)
	touch $@

.PHONY: clean_dependencies
clean_dependencies:
	rm -f $(DEPENDENCIES)/*.build $(DEPENDENCIES)/.build
	-cd $(OIS_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	rm -f $(OIS_DIR)/Makefile.in $(OIS_DIR)/aclocal.m4 $(OIS_DIR)/compile
	rm -f $(OIS_DIR)/configure $(OIS_DIR)/depcomp $(OIS_DIR)/install-sh
	-cd $(PROTOBUF_DIR) && [ -f ./Makefile ] && $(MAKE) clean
	-cd $(SFML_DIR) && $(MAKE) clean
	-cd $(ZLIB_DIR) && [ -f ./Makefile ] && $(MAKE) clean
