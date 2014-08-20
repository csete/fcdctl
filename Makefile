LIBS = `pkg-config --libs --cflags libusb-1.0` -lpthread
CC = gcc
WINLIBS= -lsetupapi
WINCC  = i686-w64-mingw32-gcc
WIN64CC= x86_64-w64-mingw32-gcc -m64
WINDLL = fcd.dll
WIN64DLL = fcd64.dll
WINSRC = fcd.c hidwin.c

SOURCE = fcd.c hid-libusb.c main.c
LIBSRC = fcd.c hid-libusb.c
EXEC = fcdctl
LIBFCD = libfcd.so

all:
	$(CC) $(SOURCE) $(LIBS) -o $(EXEC) -Wall

lib:
	$(CC) $(LIBSRC) $(LIBS) -shared -o $(LIBFCD) -Wall -fPIC

clean:
	rm -rf *.o *~ $(EXEC) $(LIBFCD) $(WINDLL) $(WIN64DLL)

win64lib:
	$(WIN64CC) $(WINSRC) -shared -o $(WIN64DLL) $(WINLIBS)

winlib:
	$(WINCC) $(WINSRC) -shared -o $(WINDLL) $(WINLIBS)
