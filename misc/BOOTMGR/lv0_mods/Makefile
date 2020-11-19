CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: bootmgr.e2xp
	
bootmgr.e2xp: bootmgr
	$(OBJCOPY) -O binary $^ $@
	
bootmgr: bootmgr.o
	$(CC) -o $@ $^ $(LDFLAGS) -T bootmgr.x
