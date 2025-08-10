LIBE2XR := "../../output/libe2xr.a"
INCE2XR := "../../blob/include" "../../blob/FatFs"
CC := arm-vita-eabi-gcc
CFLAGS := $(foreach dir,$(INCE2XR),-I$(dir)) -mcpu=cortex-a9 -mthumb-interwork -mno-unaligned-access \
	  -O1 -g3 -Wall -Wno-unused-const-variable -ffreestanding -Wno-unused-variable
OBJCOPY := arm-vita-eabi-objcopy
LDFLAGS := -nostartfiles -nostdlib -lgcc -lm -lbaremetal

all: bootmgr.e2xp
	rm bootmgr.o && rm bootmgr

bootmgr.e2xp: bootmgr
	$(OBJCOPY) -O binary $^ $@

bootmgr: bootmgr.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBE2XR) $(LDFLAGS) -T bootmgr.x

clean:
	rm -f bootmgr.o bootmgr bootmgr.e2xp
