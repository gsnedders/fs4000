CC = gcc
CFLAGS = 

C_SRC = fs4000-scsi.c scsi_via_aspi.c scsi_via_usb.c
C_OBJ = $(C_SRC,.c=.o)

INCLUDE = /usr/include/wine/windows

%.o: %.c %.h
	$(CC) -c -I$(INCLUDE) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o