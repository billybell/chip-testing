CCBASEDIR=/usr/bin

CC=$(CCBASEDIR)/gcc
CFLAGS=-Wall -g -O -I$(CCBASEDIR)/include -I/usr/include/dbus-1.0 \
 -I/usr/include/NetworkManager \
 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include/ \
 -I/usr/include/glib-2.0 \
 -I/usr/include/libnm-glib \

OBJECTS=main.o sqlite3.o

LIBS=-lpthread -ldl -ldbus-glib-1 -lglib-2.0 -lnm-util -lnm-glib -lgobject-2.0

main: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LIBS)

