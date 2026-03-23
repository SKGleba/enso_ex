INCE2XR := "../../blob/include" "../../blob/FatFs" "../../blob/lv0p"
CC=arm-vita-eabi-gcc
CFLAGS=$(foreach dir,$(INCE2XR),-I$(dir)) -mcpu=cortex-a9 -mthumb-interwork -mno-unaligned-access -fpie -fPIC \
	  -O1 -g3 -Wall -Wno-unused-const-variable -ffreestanding -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nostartfiles -nodefaultlibs -nostdlib

all: pie.e2xp
	rm pie.o && rm pie

pie.e2xp: pie
	$(OBJCOPY) -O binary $^ $@
	
pie: pie.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -T pie.x
