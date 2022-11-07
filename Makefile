HADESDBG_32_NAME=hadesdbg32
HADESDBG_64_NAME=hadesdbg64

HADESDBG_SRC=src
HADESDBG_INC=headers
HADESDBG_OUT=bin
HADESDBG_RELEASE_OUT=release
HADESDBG_OUT_LIB=bin/lib
HADESDBG_OUT=./bin
HADESDBG_TEST_SRC_32=./test/src/x86
HADESDBG_TEST_SRC_64=./test/src/x64
HADESDBG_TEST_OUT_32=./test/bin/x86
HADESDBG_TEST_OUT_64=./test/bin/x64

ASMJIT_NAME=asmjit
ASMJIT_32_FLAGS=-DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32 -DASMJIT_STATIC=ON
ASMJIT_64_FLAGS=-DCMAKE_CXX_FLAGS=-m64 -DCMAKE_SHARED_LINKER_FLAGS=-m64 -DASMJIT_STATIC=ON
ASMJIT_INC=lib/asmjit/src
ASMJIT_OUT=lib/asmjit/bin

SIMPLESON_NAME=simpleson
SIMPLESON_32_FLAGS=-DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32
SIMPLESON_64_FLAGS=-DCMAKE_CXX_FLAGS=-m64 -DCMAKE_SHARED_LINKER_FLAGS=-m64
SIMPLESON_INC=lib/simpleson
SIMPLESON_OUT=lib/simpleson/bin

LIB_32_DIR=x86
LIB_64_DIR=x64

CC=g++
COMPILE_FLAGS=-std=c++17 -lrt
RELEASE_FLAGS=-static
COMPILE_32_FLAGS=-m32
COMPILE_64_FLAGS=-m64
QUIET_MODE=> /dev/null
SILENT_MODE=2> /dev/null

HadesDbg: clean compile

bin/lib:
	@echo "Cloning libraries..."
	@rm -rf ./lib/*
	@git submodule init
	@git submodule update
	@echo "Libraries successfully cloned !"
	@echo "Compiling libraries..."
	@mkdir -p $(HADESDBG_OUT_LIB)/$(LIB_32_DIR)
	@mkdir -p $(HADESDBG_OUT_LIB)/$(LIB_64_DIR)
	@echo "Compiling asmjit..."
	@mkdir -p $(ASMJIT_OUT)
	@echo "Compiling 32bit version..."
	@cd $(ASMJIT_OUT) && sh -c 'cmake $(ASMJIT_32_FLAGS) .. $(SILENT_MODE)' $(QUIET_MODE) && make $(QUIET_MODE)
	@mv $(ASMJIT_OUT)/libasmjit.a $(HADESDBG_OUT_LIB)/$(LIB_32_DIR)/
	@rm -rf $(ASMJIT_OUT)
	@mkdir -p $(ASMJIT_OUT)
	@echo "Compiling 64bit version..."
	@cd $(ASMJIT_OUT) && sh -c 'cmake $(ASMJIT_64_FLAGS) .. $(SILENT_MODE)' $(QUIET_MODE) && make $(QUIET_MODE)
	@mv $(ASMJIT_OUT)/libasmjit.a $(HADESDBG_OUT_LIB)/$(LIB_64_DIR)/
	@rm -rf $(ASMJIT_OUT)
	@echo "Compiling simpleson..."
	@mkdir -p $(SIMPLESON_OUT)
	@echo "Compiling 32bit version..."
	@cd $(SIMPLESON_OUT) && sh -c 'cmake $(SIMPLESON_32_FLAGS) .. $(SILENT_MODE)' $(QUIET_MODE) && make $(QUIET_MODE)
	@mv $(SIMPLESON_OUT)/libsimpleson.a $(HADESDBG_OUT_LIB)/$(LIB_32_DIR)/
	@rm -rf $(SIMPLESON_OUT)
	@mkdir -p $(SIMPLESON_OUT)
	@echo "Compiling 64bit version..."
	@cd $(SIMPLESON_OUT) && sh -c 'cmake $(SIMPLESON_64_FLAGS) .. $(SILENT_MODE)' $(QUIET_MODE) && make $(QUIET_MODE)
	@mv $(SIMPLESON_OUT)/libsimpleson.a $(HADESDBG_OUT_LIB)/$(LIB_64_DIR)/
	@rm -rf $(SIMPLESON_OUT)
	@echo "Libraries successfully compiled !"

.PHONY: compile
compile: bin/lib
	@echo "Compiling project..."
	@mkdir -p $(HADESDBG_OUT)
	@echo "Compiling 32bit version..."
	@$(CC) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(ASMJIT_INC) -I$(SIMPLESON_INC) -o$(HADESDBG_OUT)/$(HADESDBG_32_NAME) $(COMPILE_FLAGS) $(COMPILE_32_FLAGS) -L"$(HADESDBG_OUT_LIB)/$(LIB_32_DIR)" -l$(ASMJIT_NAME) -l$(SIMPLESON_NAME) -Wl,-rpath,"`pwd`/$(HADESDBG_OUT_LIB)/$(LIB_32_DIR)"
	@chmod 777 $(HADESDBG_OUT)/$(HADESDBG_32_NAME)
	@echo "Compiling 64bit version..."
	@$(CC) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(ASMJIT_INC) -I$(SIMPLESON_INC) -o$(HADESDBG_OUT)/$(HADESDBG_64_NAME) $(COMPILE_FLAGS) $(COMPILE_64_FLAGS) -L"$(HADESDBG_OUT_LIB)/$(LIB_64_DIR)" -l$(ASMJIT_NAME) -l$(SIMPLESON_NAME) -Wl,-rpath,"`pwd`/$(HADESDBG_OUT_LIB)/$(LIB_64_DIR)"
	@chmod 777 $(HADESDBG_OUT)/$(HADESDBG_64_NAME)
	@echo "Project successfully compiled !"

.PHONY: release
release: bin/lib
	@echo "Releasing project..."
	@mkdir -p $(HADESDBG_RELEASE_OUT)
	@echo "Releasing 32bit version..."
	@$(CC) $(RELEASE_FLAGS) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(ASMJIT_INC) -I$(SIMPLESON_INC) -o$(HADESDBG_RELEASE_OUT)/$(HADESDBG_32_NAME) $(COMPILE_FLAGS) $(COMPILE_32_FLAGS) -L"$(HADESDBG_OUT_LIB)/$(LIB_32_DIR)" -l$(ASMJIT_NAME) -l$(SIMPLESON_NAME)
	@chmod 777 $(HADESDBG_RELEASE_OUT)/$(HADESDBG_32_NAME)
	@echo "Releasing 64bit version..."
	@$(CC) $(RELEASE_FLAGS) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(ASMJIT_INC) -I$(SIMPLESON_INC) -o$(HADESDBG_RELEASE_OUT)/$(HADESDBG_64_NAME) $(COMPILE_FLAGS) $(COMPILE_64_FLAGS) -L"$(HADESDBG_OUT_LIB)/$(LIB_64_DIR)" -l$(ASMJIT_NAME) -l$(SIMPLESON_NAME)
	@chmod 777 $(HADESDBG_RELEASE_OUT)/$(HADESDBG_64_NAME)
	@echo "Building zip archive..."
	@cd $(HADESDBG_RELEASE_OUT) && zip ../HadesDbg.zip $(HADESDBG_32_NAME) $(HADESDBG_64_NAME)
	@rm -rf $(HADESDBG_RELEASE_OUT)
	@echo "Project successfully released !"
	
.PHONY: test
test:
	@mkdir -p $(HADESDBG_TEST_OUT_32)
	@mkdir -p $(HADESDBG_TEST_OUT_64)
	@echo "Compiling 32bit test binaries..."
	@for test_src_file in $(HADESDBG_TEST_SRC_32)/* ; do \
		$(CC) $(COMPILE_FLAGS) $(COMPILE_32_FLAGS) $$test_src_file -o$(HADESDBG_TEST_OUT_32)/`echo $$test_src_file | rev | cut -d'/' -f 1 | rev | cut -d'.' -f 1)` ; \
	done
	@echo "Compiling 64bit test binaries..."
	@for test_src_file in $(HADESDBG_TEST_SRC_64)/* ; do \
		$(CC) $(COMPILE_FLAGS) $(COMPILE_64_FLAGS) $$test_src_file -o$(HADESDBG_TEST_OUT_64)/`echo $$test_src_file | rev | cut -d'/' -f 1 | rev | cut -d'.' -f 1)` ; \
	done
	@echo "Done !"

.PHONY:	clean
clean:
	@echo "Cleaning output directory..."
	@rm -f $(HADESDBG_OUT)/$(HADESDBG_32_NAME)
	@rm -f $(HADESDBG_OUT)/$(HADESDBG_64_NAME)
