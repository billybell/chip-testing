CCBASEDIR=/usr/bin

CC=$(CCBASEDIR)/gcc
CFLAGS=-Wall -O -I$(CCBASEDIR)/include

OBJECTS=main.o sqlite3.o

LIBS=-lpthread -ldl

main: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LIBS)

