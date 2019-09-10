CC = gcc


CFLAGS = -std=gnu99 -Wall -pedantic -I./include -I/usr/include -I/usr/include/libusb-1.0 -I/usr/local/include -I/opt/local/include

 
LIBS =  -lusb-1.0


all: bin/msxGameReader

clean:
	rm -f obj/*.o bin/*msxGameReader*



OFILES += obj/msxGameReader.o
obj/msxGameReader.o: source/msxGameReader.c 
	@mkdir -p obj
	$(CC) $(CFLAGS) -c source/msxGameReader.c -o obj/msxGameReader.o

	
OFILES += obj/debugnet.o
obj/debugnet.o: source/debugnet.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c source/debugnet.c -o obj/debugnet.o




 bin/msxGameReader: $(OFILES)  
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OFILES)  -o bin/msxGameReader $(LIBS)
