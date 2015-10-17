CCBASEDIR=/home/vagrant/CHIP-buildroot/output/host/usr

CC=$(CCBASEDIR)/bin/arm-linux-gnueabihf-gcc
CFLAGS=-Wall -I$(CCBASEDIR)/include

all: main
main: main.c
	$(CC) main.c $(CFLAGS) -o main
clean:
	rm -f main
