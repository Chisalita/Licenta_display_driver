CC=gcc
CFLAGS=-Wall -std=c99
#DEPS = hellomake.h
OBJ = main.o #more here 

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm ./*.o
