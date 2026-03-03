#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "idt.h"
#include "shell.h"
#include "string.h"

// Entry point - called from bootloader
void kernel_main(void) __attribute__((section(".text.start")));

void kernel_main(void) {
    // Initialize VGA
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();

    // Initialize keyboard
    keyboard_init();

    // Initialize IDT and enable interrupts
    idt_init();

    // Initialize shell and filesystem
    shell_init();

    // Run shell
    shell_run();

    // Should never reach here
    while (1) {
        __asm__ volatile("hlt");
    }
}