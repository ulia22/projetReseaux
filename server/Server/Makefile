CC = gcc
BIN = server
SRC = main.c fileData.c pair.c
OBJ = main.o fileData.o pair.o
LDLIBS = -lpthread

ERROR_CFLAGS = -Wall -W -pedantic
 

 
 

all: $(BIN)
 
$(BIN): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJ) $(LDLIBS)