LIBS = `pkg-config --libs --cflags libusb-1.0`
CC = gcc

SOURCE = fcd.c hid-libusb.c main.c
EXEC = fcdctl
PREFIX = /usr/local

all:
	$(CC) $(LIBS) $(SOURCE) -o $(EXEC)

clean:
	rm -rf *.o *~ $(EXEC) 

install:
	cp $(EXEC) $(PREFIX)/bin/
