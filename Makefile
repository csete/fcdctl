LIBS = `pkg-config --libs --cflags libusb-1.0` -lpthread
CC = gcc

SOURCE = fcd.c hid-libusb.c main.c
LIBSRC = fcd.c hid-libusb.c
EXEC = fcdctl
LIBFCD = libfcd.so

all:
	$(CC) $(SOURCE) $(LIBS) -o $(EXEC) -Wall

lib:
	$(CC) $(LIBSRC) $(LIBS) -shared -o $(LIBFCD) -Wall -fPIC

clean:
	rm -rf *.o *~ $(EXEC) 
