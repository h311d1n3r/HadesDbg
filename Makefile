HADESDBG_NAME=hadesdbg

HADESDBG_SRC=./src
HADESDBG_INC=./headers
HADESDBG_OUT=./bin
HADESDBG_OUT_LIB=./bin/lib

ASMJIT_DIR=./lib/asmjit
ASMJIT_INC=./lib/asmjit/src
ASMJIT_OUT=./lib/asmjit/bin
ASMJIT_URL=https://github.com/asmjit/asmjit.git
ASMJIT_COMMIT=a4cb51b532af0f8137c4182914244c3b05d7745f

LIB_INC=$(ASMJIT_INC)

CC=g++
COMPILE_FLAGS=-std=c++17 -lrt
QUIET_MODE=> /dev/null
SILENT_MODE=2> /dev/null

HadesDbg: clean compile

bin/lib:
	@echo "Cloning libraries..."
	@rm -rf ./lib/*
	@echo "Cloning asmjit..."
	@cd ./lib && sh -c 'git clone $(ASMJIT_URL) $(SILENT_MODE)' $(QUIET_MODE)
	@cd $(ASMJIT_DIR) && sh -c 'git checkout $(ASMJIT_COMMIT) $(SILENT_MODE)' $(QUIET_MODE)
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
	@$(CC) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -I$(LIB_INC) -o$(HADESDBG_OUT)/$(HADESDBG_NAME) $(COMPILE_FLAGS) -L$(HADESDBG_OUT_LIB) -lasmjit -Wl,-rpath,$(HADESDBG_OUT_LIB)
	@chmod 777 $(HADESDBG_OUT)/$(HADESDBG_NAME)
	@echo "Project successfully compiled !"
			  
clean:
	@echo "Cleaning output directory..."
	@rm -f $(HADESDBG_OUT)/$(HADESDBG_NAME)
