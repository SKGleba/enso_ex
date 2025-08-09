CC=arm-vita-eabi-gcc
CFLAGS=-O1 -fno-builtin-printf -fno-builtin-memset -Wall -Wextra -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: bootmgr.e2xp
	rm bootmgr.o && rm bootmgr
	
bootmgr.e2xp: bootmgr
	$(OBJCOPY) -O binary $^ $@
	
bootmgr: bootmgr.o
	$(CC) -o $@ $^ $(LDFLAGS) -T bootmgr.x
