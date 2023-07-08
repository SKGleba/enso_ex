CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: fat.bin
	rm second.o && rm first.o && rm second && rm first && rm first.bin && rm second.bin

fat.bin: first.bin second.bin
	./gen.py fat.tpl first.bin second.bin fat.bin

first.bin: first
	$(OBJCOPY) -O binary $^ $@

second.bin: second
	$(OBJCOPY) -O binary $^ $@

first: first.o
	$(CC) -o $@ $^ $(LDFLAGS) -T first.x

second: second.o
	$(CC) -o $@ $^ $(LDFLAGS) -T second.x
