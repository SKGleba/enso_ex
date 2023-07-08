CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: recovery.e2xr
	rm recovery && rm recovery.o
	
recovery.e2xr: recovery
	$(OBJCOPY) -O binary $^ $@
	
recovery: recovery.o
	$(CC) -o $@ $^ $(LDFLAGS) -T recovery.x
