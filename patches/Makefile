CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: clogo_$(FW).e2xp rconfig_$(FW).e2xp
	rm *.o && rm clogo_$(FW) && rm rconfig_$(FW)
	
clogo_$(FW).e2xp: clogo_$(FW)
	$(OBJCOPY) -O binary $^ $@

rconfig_$(FW).e2xp: rconfig_$(FW)
	$(OBJCOPY) -O binary $^ $@
	
clogo_$(FW): clogo.o
	$(CC) -o $@ $^ $(LDFLAGS)
	
rconfig_$(FW): rconfig.o
	$(CC) -o $@ $^ $(LDFLAGS)
