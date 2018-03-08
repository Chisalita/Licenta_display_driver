CC=gcc
CFLAGS=-pthread -lpigpio -Wall -std=c99
#DEPS = hellomake.h
OBJ = main.o #more here 
APPLICATION_NAME = application


%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

$(APPLICATION_NAME): $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm ./*.o ./*.out $(APPLICATION_NAME)
