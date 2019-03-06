ITEMS	 := 22
THREADS	 := 3
SHOWDATA := 1


CC		:= gcc

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

LIBRARIES	:=

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= parallelout.exe
else
EXECUTABLE	:= parallelout
endif

all: $(BIN)/$(EXECUTABLE)

clean:
	-$(RM) $(BIN)/$(EXECUTABLE)

run: all
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*
	$(CC) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES) -DNITEMS=$(ITEMS) -DNTHREADS=$(THREADS) -DSHOWDATA=$(SHOWDATA) -lpthread