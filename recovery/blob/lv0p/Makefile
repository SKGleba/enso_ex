PSP2REF_DIR=../../../../psp2ref

PREFIX=mep-elf-
CC=$(PREFIX)gcc
CFLAGS=-I$(PSP2REF_DIR) -fno-delete-null-pointer-checks -nostdlib -fno-optimize-sibling-calls -O1 -std=gnu99 -mc=far -mel -ml
LD=$(PREFIX)gcc
LDFLAGS= -nodefaultlibs -nostdlib -ffreestanding -Tlv0p.x
OBJCOPY=$(PREFIX)objcopy
OBJCOPYFLAGS=

OBJ=lv0p.o

all: lv0p_data.h

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.ao: %.S
	$(CC) -c -o $@ $< $(CFLAGS)

lv0p.elf: $(OBJ)
	$(LD) -o $@ $^ $(LDFLAGS)

%.nmp: %.elf
	$(OBJCOPY) -O binary $< $@
	
lv0p_data.h: lv0p.nmp
	xxd -i $< > $@
	rm *.elf
	rm *.o
	# rm *.nmp

clean:
	-rm *.nmp
	-rm lv0p_data.h
	-rm *.o
	-rm *.elf