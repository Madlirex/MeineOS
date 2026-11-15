.PHONY: all run debug clean

ASM     = nasm
CXX     = x86_64-elf-g++
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU    = qemu-system-x86_64

CXXFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -mno-red-zone -mcmodel=kernel

all: os.img

stage1.bin: boot/stage1.asm
	$(ASM) -f bin $< -o $@

stage2.bin: boot/stage2.asm
	$(ASM) -f bin $< -o $@

CPP_SOURCES = $(wildcard kernel/*.cpp)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

kernel/%.o: kernel/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

kernel.elf: $(CPP_OBJECTS) linker.ld
	$(LD) -T linker.ld -o $@ $(CPP_OBJECTS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

os.img: stage1.bin stage2.bin kernel.bin
	cat stage1.bin stage2.bin kernel.bin > os.img
	fsutil file seteof os.img 1474560

run: os.img
	$(QEMU) -drive format=raw,file=os.img -serial stdio

debug: os.img
	$(QEMU) -drive format=raw,file=os.img -s -S -no-reboot -d int

clean:
	rm -f *.bin *.o *.elf *.img
