CC = gcc
CFLAGS = -Wall

C_SRC = fs4000-scsi.c scsi_via_usb.c
C_OBJ = $(C_SRC,.c=.o)

INCLUDE = -I/usr/include/wine/windows -I/usr/include/libusb-1.0

%.o: %.c %.h
	$(CC) -c $(INCLUDE) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o
