LIBS = `pkg-config --libs --cflags libusb-1.0` -lpthread
CC = gcc

SOURCE = fcd.c hid-libusb.c main.c
EXEC = fcdctl

all:
	$(CC) $(SOURCE) $(LIBS) -o $(EXEC) -Wall

clean:
	rm -rf *.o *~ $(EXEC) 
