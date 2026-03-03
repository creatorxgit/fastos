#!/bin/bash
set -e

echo "========================================="
echo "  FastOS Build System"
echo "========================================="

# Check dependencies
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo "ERROR: $1 is not installed!"
        echo "Install it with:"
        case $1 in
            nasm)
                echo "  sudo apt install nasm"
                ;;
            gcc)
                echo "  sudo apt install gcc"
                ;;
            qemu-system-i386)
                echo "  sudo apt install qemu-system-x86"
                ;;
            xorriso)
                echo "  sudo apt install xorriso"
                ;;
        esac
        exit 1
    fi
}

echo "[1/6] Checking dependencies..."
check_tool nasm
check_tool gcc
check_tool ld
check_tool xorriso
echo "  All dependencies found."

echo "[2/6] Cleaning previous build..."
rm -f kernel/*.o boot/*.bin kernel/*.bin fastos.img fastos.iso
rm -rf iso_root

echo "[3/6] Assembling bootloader..."
nasm -f bin boot/boot.asm -o boot/boot.bin
echo "  boot.bin: $(wc -c < boot/boot.bin) bytes"

echo "[4/6] Compiling kernel..."
# Compile all C files
for src in kernel/kernel.c kernel/vga.c kernel/keyboard.c \
           kernel/string.c kernel/idt.c kernel/ports.c \
           kernel/filesystem.c kernel/shell.c; do
    obj="${src%.c}.o"
    echo "  Compiling $src..."
    gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin \
        -nostdlib -nostdinc -Wall -Wextra -O2 -c "$src" -o "$obj"
done

# Compile ASM
echo "  Assembling idt_asm.asm..."
nasm -f elf32 kernel/idt_asm.asm -o kernel/idt_asm.o

echo "[5/6] Linking kernel..."
ld -m elf_i386 -T kernel/linker.ld --oformat binary -nostdlib \
   -o kernel/kernel.bin \
   kernel/kernel.o kernel/vga.o kernel/keyboard.o \
   kernel/string.o kernel/idt.o kernel/ports.o \
   kernel/filesystem.o kernel/shell.o kernel/idt_asm.o

KERNEL_SIZE=$(wc -c < kernel/kernel.bin)
echo "  kernel.bin: $KERNEL_SIZE bytes"

# Check kernel isn't too big (60 sectors = 30720 bytes)
if [ "$KERNEL_SIZE" -gt 30720 ]; then
    echo "WARNING: Kernel is larger than 30KB!"
    echo "You may need to increase the sector count in boot.asm"
fi

echo "[6/6] Creating disk images..."

# Create floppy image
dd if=/dev/zero of=fastos.img bs=512 count=2880 2>/dev/null
dd if=boot/boot.bin of=fastos.img conv=notrunc bs=512 count=1 2>/dev/null
dd if=kernel/kernel.bin of=fastos.img conv=notrunc bs=512 seek=1 2>/dev/null
echo "  fastos.img created (1.44MB floppy image)"

# Create ISO
mkdir -p iso_root
cp fastos.img iso_root/
xorriso -as mkisofs \
    -b fastos.img \
    -no-emul-boot \
    -boot-load-size 2880 \
    -o fastos.iso \
    iso_root/ 2>/dev/null
rm -rf iso_root
echo "  fastos.iso created"

echo ""
echo "========================================="
echo "  Build complete!"
echo "========================================="
echo ""
echo "Files:"
echo "  fastos.img - Floppy disk image (for QEMU)"
echo "  fastos.iso - ISO image (for burning/VM)"
echo ""
echo "To run in QEMU:"
echo "  qemu-system-i386 -fda fastos.img"
echo "  qemu-system-i386 -cdrom fastos.iso"
echo ""
echo "To write to USB:"
echo "  sudo dd if=fastos.img of=/dev/sdX bs=512"
echo ""