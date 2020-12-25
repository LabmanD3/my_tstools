CC = g++
BIN = test
OBJ = main.o
INC = include/

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN)

%.o: %.c $() 



.PHONY: clean
clean:
	rm *.o $(BIN)
