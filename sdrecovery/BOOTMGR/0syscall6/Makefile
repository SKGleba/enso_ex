CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: bootmgr_$(FW).e2xp
	
bootmgr_$(FW).e2xp: bootmgr_$(FW)
	$(OBJCOPY) -O binary $^ $@
	
bootmgr_$(FW): bootmgr.o
	$(CC) -o $@ $^ $(LDFLAGS) -T bootmgr.x
