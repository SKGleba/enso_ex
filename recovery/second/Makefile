CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -Wno-multichar -fdiagnostics-color=always
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: second.bin
	rm -f *.o second

second.bin: second
	$(OBJCOPY) -O binary $^ $@

second: second.o
	$(CC) -o $@ $^ $(LDFLAGS) -T second.x

clean:
	-rm -f *.o *.bin second
