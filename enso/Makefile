CC=arm-vita-eabi-gcc
FW=365
CFLAGS=-Os -fno-builtin-printf -fPIC -fno-builtin-memset -Wall -Wextra -Wno-unused-variable -DFW_$(FW)
OBJCOPY=arm-vita-eabi-objcopy
LDFLAGS=-nodefaultlibs -nostdlib

all: fat_$(FW).bin
	rm first_* && rm second_* && rm second.o && rm $(FW)/first.o

fat_$(FW).bin: first_$(FW).bin second_$(FW).bin
	./$(FW)/gen.py $(FW)/fat.tpl first_$(FW).bin second_$(FW).bin fat_$(FW).bin

first_$(FW).bin: first_$(FW)
	$(OBJCOPY) -O binary $^ $@

second_$(FW).bin: second_$(FW)
	$(OBJCOPY) -O binary $^ $@

first_$(FW): $(FW)/first.o
	$(CC) -o $@ $^ $(LDFLAGS) -T first.x

second_$(FW): second.o
	$(CC) -o $@ $^ $(LDFLAGS) -T second.x
