CC=arm-vita-eabi-gcc
CFLAGS=-O1 -fno-builtin-printf -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -fpie -fPIC
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: pie.e2xp
	rm pie.o && rm pie

pie.e2xp: pie
	$(OBJCOPY) -O binary $^ $@
	
pie: pie.o
	$(CC) -o $@ $^ $(LDFLAGS) -T pie.x
