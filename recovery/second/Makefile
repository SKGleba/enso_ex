CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -Wno-multichar
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: fat.bin
	rm -f *.o second

fat.bin: second.bin
	touch fat.bin
	truncate -s 2560 fat.bin
	cat second.bin >> fat.bin

second.bin: second
	$(OBJCOPY) -O binary $^ $@

second: second.o
	$(CC) -o $@ $^ $(LDFLAGS) -T second.x

clean:
	-rm -f *.o *.bin second
