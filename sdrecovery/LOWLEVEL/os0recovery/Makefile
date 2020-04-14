CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: recovery_$(FW).h

recovery_$(FW).h: recovery_$(FW).e2xp
	xxd -i $< > $@
	
recovery_$(FW).e2xp: recovery_$(FW)
	$(OBJCOPY) -O binary $^ $@
	
recovery_$(FW): recovery.o
	$(CC) -o $@ $^ $(LDFLAGS) -T recovery.x
