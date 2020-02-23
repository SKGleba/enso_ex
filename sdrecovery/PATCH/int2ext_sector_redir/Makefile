CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: sector_int2ext_$(FW).e2xp
	
sector_int2ext_$(FW).e2xp: sector_int2ext_$(FW)
	$(OBJCOPY) -O binary $^ $@
	
sector_int2ext_$(FW): sector_int2ext.o
	$(CC) -o $@ $^ $(LDFLAGS) -T sector_int2ext.x
