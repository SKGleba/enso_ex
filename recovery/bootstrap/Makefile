CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -fpie -fdiagnostics-color=always
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: rconfig.e2xr
	rm -f *.o

rconfig.e2xr: bootstrap.elf
	$(OBJCOPY) -O binary $^ $@
	
bootstrap.elf: bootstrap.o
	$(CC) -o $@ $^ $(LDFLAGS) -T bootstrap.x

clean:
	rm -f *.o *.elf *.e2xr