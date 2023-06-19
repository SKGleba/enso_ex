CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -fpie
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: rbootstrap.e2xp
	
rbootstrap.e2xp: bootstrap.elf
	$(OBJCOPY) -O binary $^ $@
	
bootstrap.elf: bootstrap.o
	$(CC) -o $@ $^ $(LDFLAGS)
