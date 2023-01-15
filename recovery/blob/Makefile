CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -fpie -mcpu=cortex-a9
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: rblob.e2xp
	
rblob.e2xp: blob.elf
	$(OBJCOPY) -O binary $^ $@
	
blob.elf: blob.o
	$(CC) -o $@ $^ $(LDFLAGS)
