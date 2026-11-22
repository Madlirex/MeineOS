.PHONY: all run debug clean

ASM = nasm
CXX = x86_64-elf-g++
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU = qemu-system-x86_64

CXXFLAGS = -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=kernel

all: os.img

stage1.bin: boot/stage1.asm
	$(ASM) -f bin $< -o $@

stage2.bin: boot/stage2.asm
	$(ASM) -f bin $< -o $@

CPP_SOURCES = $(wildcard kernel/*.cpp)
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

kernel/interrupts.o: kernel/interrupts.asm
	$(ASM) -f elf64 $< -o $@

kernel/%.o: kernel/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

kernel.elf: $(CPP_OBJECTS) kernel/interrupts.o linker.ld
	$(LD) -n -z noexecstack -T linker.ld -o $@ $(CPP_OBJECTS) kernel/interrupts.o

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

os.img: stage1.bin stage2.bin kernel.bin
	cat stage1.bin stage2.bin kernel.bin > os.img
	fsutil file seteof os.img 1474560

run: os.img
	$(QEMU) -m 8G -drive format=raw,file=os.img -serial stdio

debug: os.img
	$(QEMU) -drive format=raw,file=os.img -s -S -no-reboot -d int -serial stdio

clean:
	rm -f *.bin kernel/*.o *.o *.elf *.img
