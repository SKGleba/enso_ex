PREFIX=mep-elf-
CC=$(PREFIX)gcc
CFLAGS=-fno-delete-null-pointer-checks -nostdlib -fno-optimize-sibling-calls -mc=tiny -Os -std=gnu99 -mel
LOADER_CFLAGS=-nostdlib -mc=far -mtf -ml -Os -std=gnu99 -mel
LD=$(PREFIX)gcc
LDFLAGS= -nodefaultlibs -nostdlib
OBJCOPY=$(PREFIX)objcopy
OBJCOPYFLAGS=

OBJ=main.o

all: dex_patch.h

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.ao: %.S
	$(CC) -c -o $@ $< $(CFLAGS)

dex_patch.elf: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

%.nmp: %.elf
	$(OBJCOPY) -O binary $< $@
	
%.h: %.nmp
	xxd -i $< > $@
	rm *.elf
	rm *.o