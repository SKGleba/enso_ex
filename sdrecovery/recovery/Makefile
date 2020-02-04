CC=arm-vita-eabi-gcc
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_365
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: recovery.e2xs
	
recovery.e2xs: recovery
	$(OBJCOPY) -O binary $^ $@
	
recovery: recovery.o
	$(CC) -o $@ $^ $(LDFLAGS)
