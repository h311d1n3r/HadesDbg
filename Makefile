HADESDBG_NAME=hadesdbg

HADESDBG_SRC=src
HADESDBG_INC=headers
HADESDBG_OUT=bin
HADESDBG_OUT_LIB=bin/lib
HADESDBG_OUT=./bin
HADESDBG_TEST_SRC_32=./test/src/x86
HADESDBG_TEST_SRC_64=./test/src/x64
HADESDBG_TEST_OUT=./test/bin

ASMJIT_DIR=lib/asmjit
ASMJIT_INC=lib/asmjit/src
ASMJIT_OUT=lib/asmjit/bin

LIB_INC=$(ASMJIT_INC)

CC=g++
COMPILE_FLAGS=-std=c++17 -lrt
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
	@mkdir -p $(HADESDBG_OUT_LIB)
	@echo "Compiling asmjit..."
	@mkdir -p $(ASMJIT_OUT)
	@cd $(ASMJIT_OUT) && sh -c 'cmake .. $(SILENT_MODE)' $(QUIET_MODE) && make $(QUIET_MODE)
	@mv $(ASMJIT_OUT)/libasmjit.so $(HADESDBG_OUT_LIB)
	@rm -rf $(ASMJIT_OUT)
	@echo "Libraries successfully compiled !"

compile: bin/lib
	@echo "Compiling project..."
	@mkdir -p $(HADESDBG_OUT)
	@$(CC) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(LIB_INC) -o$(HADESDBG_OUT)/$(HADESDBG_NAME) $(COMPILE_FLAGS) -L"`pwd`/$(HADESDBG_OUT_LIB)" -lasmjit -Wl,-rpath,"`pwd`/$(HADESDBG_OUT_LIB)"
	@chmod 777 $(HADESDBG_OUT)/$(HADESDBG_NAME)
	@echo "Project successfully compiled !"

.PHONY: test
test:
	@mkdir -p $(HADESDBG_TEST_OUT)
	@echo "Compiling test binaries..."
	#@for test_src_file in $(HADESDBG_TEST_SRC_32)/* ; do \
	#	$(CC) $(COMPILE_FLAGS) $(COMPILE_32_FLAGS) $$test_src_file -o$(HADESDBG_TEST_OUT)/`echo $$test_src_file | rev | cut -d'/' -f 1 | rev | cut -d'.' -f 1)` ; \
	#done
	@for test_src_file in $(HADESDBG_TEST_SRC_64)/* ; do \
		$(CC) $(COMPILE_FLAGS) $(COMPILE_64_FLAGS) $$test_src_file -o$(HADESDBG_TEST_OUT)/`echo $$test_src_file | rev | cut -d'/' -f 1 | rev | cut -d'.' -f 1)` ; \
	done
	@echo "Done !"
			  
clean:
	@echo "Cleaning output directory..."
	@rm -f $(HADESDBG_OUT)/$(HADESDBG_NAME)
