CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: recovery.h

recovery.h: recovery.e2xp
	xxd -i $< > $@
	
recovery.e2xp: recovery
	$(OBJCOPY) -O binary $^ $@
	
recovery: recovery.o
	$(CC) -o $@ $^ $(LDFLAGS) -T recovery.x
