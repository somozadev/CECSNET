CC=gcc
CFLAGS=-Wall -Iinclude
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
OUT=ecsnet

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $^

clean:
	rm -f $(OUT) src/*.o
