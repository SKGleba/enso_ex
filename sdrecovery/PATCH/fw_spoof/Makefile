CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: fwspoof_$(FW).e2xp
	
fwspoof_$(FW).e2xp: fwspoof_$(FW)
	$(OBJCOPY) -O binary $^ $@
	
fwspoof_$(FW): fwspoof.o
	$(CC) -o $@ $^ $(LDFLAGS) -T fwspoof.x
