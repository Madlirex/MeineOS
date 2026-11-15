.PHONY: all run debug clean

ASM     = nasm
CXX     = x86_64-elf-g++
LD      = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU    = qemu-system-x86_64

CXXFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -mno-red-zone -mcmodel=kernel

all: os.img

boot.bin: boot/boot.asm
	$(ASM) -f bin $< -o $@

kernel.o: kernel/kernel.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

kernel.elf: kernel.o linker.ld
	$(LD) -T linker.ld -o $@ kernel.o

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.img
	fsutil file seteof os.img 1474560

run: os.img
	$(QEMU) -drive format=raw,file=os.img -serial stdio

debug: os.img
	$(QEMU) -drive format=raw,file=os.img -s -S -no-reboot -d int

clean:
	del /Q *.bin *.o *.elf *.img 2>nul || exit /b 0
