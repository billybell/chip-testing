CCBASEDIR=/home/vagrant/CHIP-buildroot/output/host/usr

CC=$(CCBASEDIR)/bin/arm-linux-gnueabihf-gcc
CFLAGS=-Wall -O -I$(CCBASEDIR)/include

OBJECTS=main.o sqlite3.o

LIBS=-lpthread -ldl

main: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LIBS)

