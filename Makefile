HADESDBG_NAME=hadesdbg

HADESDBG_SRC=./src
HADESDBG_INC=./headers
HADESDBG_OUT=./bin
HADESDBG_OUT_LIB=./bin/lib

ASMJIT_INC=./lib/asmjit/src
ASMJIT_OUT=./lib/asmjit/bin

LIB_INC=$(ASMJIT_INC)

CC=g++
COMPILE_FLAGS=-std=c++17 -lrt
QUIET_MODE=> /dev/null

HADESDBG: clean compile

bin/lib:
	@echo "Compiling libraries..."
	@mkdir -p $(HADESDBG_OUT_LIB)
	@echo "Compiling asmjit..."
	@mkdir -p $(ASMJIT_OUT)
	@cd $(ASMJIT_OUT) && sh -c 'cmake .. 2>/dev/null' $(QUIET_MODE) && make $(QUIET_MODE)
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
