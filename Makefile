HADESDBG_NAME=hadesdbg

HADESDBG_SRC=./src
HADESDBG_INC=./headers
HADESDBG_OUT=./bin

CC=g++
COMPILE_FLAGS=-std=c++17

HADESDBG:	clean compile

compile:
	mkdir -p $(HADESDBG_OUT)
	$(CC) $(COMPILE_FLAGS) $(HADESDBG_SRC)/*.cpp -I$(HADESDBG_INC) -o$(HADESDBG_OUT)/$(HADESDBG_NAME)
			  
clean:
	rm -rf $(HADESDBG_OUT)
