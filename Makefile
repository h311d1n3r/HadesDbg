HADESDBG_NAME=hadesdbg

HADESDBG_SRC=./src
HADESDBG_INC=./headers
HADESDBG_OUT=./bin

ASMJIT_SRC=./lib/asmjit/src/asmjit/**
ASMJIT_INC=./lib/asmjit/src/asmjit

LIB_SRC=$(ASMJIT_SRC)
LIB_INC=$(ASMJIT_INC)

CC=g++
COMPILE_FLAGS=-std=c++17 -lrt

HADESDBG:	clean compile

compile:
	@echo "Compiling..."
	@mkdir -p $(HADESDBG_OUT)
	@$(CC) $(HADESDBG_SRC)/*.cpp $(LIB_SRC)/*.cpp -I$(HADESDBG_INC) -I$(LIB_INC) -o$(HADESDBG_OUT)/$(HADESDBG_NAME) $(COMPILE_FLAGS)
			  
clean:
	@echo "Cleaning output directory..."
	@rm -rf $(HADESDBG_OUT)
