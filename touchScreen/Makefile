CC=gcc
CFLAGS=-pthread -lpigpio -Wall -std=c99
#DEPS = hellomake.h
OBJ = main.o #more here 

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

touch: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm ./*.o
