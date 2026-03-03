# FAST OS
## Why FAST OS?
### 1.It easy to use
### 2.Its super fast
### 3.You can delete, add, edit and read files.
### 4. Its full open source
## LICENSE
### You can copy, change and share this OS to anyone.

#
#
#
#
# Installation

# FastOS Makefile
# Requires: nasm, i686-elf-gcc (or gcc with -m32), ld, xorriso, grub-mkrescue

# Tools - adjust for your cross-compiler
CC = gcc
AS = nasm
LD = ld

# If you have a cross-compiler, use these instead:
# CC = i686-elf-gcc
# LD = i686-elf-ld

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin \
         -nostdlib -nostdinc -Wall -Wextra -O2 -c
LDFLAGS = -m elf_i386 -T kernel/linker.ld --oformat binary -nostdlib

KERNEL_SOURCES = kernel/kernel.c kernel/vga.c kernel/keyboard.c \
                 kernel/string.c kernel/idt.c kernel/ports.c \
                 kernel/filesystem.c kernel/shell.c

KERNEL_OBJECTS = $(KERNEL_SOURCES:.c=.o)
ASM_OBJECTS = kernel/idt_asm.o

.PHONY: all clean iso run

all: fastos.iso

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Compile ASM interrupt handler
kernel/idt_asm.o: kernel/idt_asm.asm
	$(AS) -f elf32 $< -o $@

# Assemble bootloader
boot/boot.bin: boot/boot.asm
	$(AS) -f bin $< -o $@

# Link kernel
kernel/kernel.bin: $(KERNEL_OBJECTS) $(ASM_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJECTS) $(ASM_OBJECTS)

# Create raw disk image
fastos.img: boot/boot.bin kernel/kernel.bin
	# Create 1.44MB floppy image
	dd if=/dev/zero of=$@ bs=512 count=2880
	# Write bootloader to first sector
	dd if=boot/boot.bin of=$@ conv=notrunc bs=512 count=1
	# Write kernel starting at sector 2
	dd if=kernel/kernel.bin of=$@ conv=notrunc bs=512 seek=1

# Create ISO using xorriso (El Torito boot)
fastos.iso: fastos.img
	mkdir -p iso_root
	cp fastos.img iso_root/
	xorriso -as mkisofs \
		-b fastos.img \
		-no-emul-boot \
		-boot-load-size 2880 \
		-o $@ \
		iso_root/
	rm -rf iso_root

# Run in QEMU
run: fastos.img
	qemu-system-i386 -fda fastos.img -boot a

# Run ISO in QEMU
run-iso: fastos.iso
	qemu-system-i386 -cdrom fastos.iso -boot d

# Run with serial console for debugging
debug: fastos.img
	qemu-system-i386 -fda fastos.img -boot a -serial stdio -d int,cpu_reset

clean:
	rm -f kernel/*.o boot/*.bin kernel/*.bin fastos.img fastos.iso
	rm -rf iso_root
