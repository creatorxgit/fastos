#include "idt.h"
#include "ports.h"
#include "keyboard.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    memset(&idt, 0, sizeof(idt));

    // Remap PIC
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    port_byte_out(0x21, 0x20); // IRQ 0-7 -> INT 0x20-0x27
    port_byte_out(0xA1, 0x28); // IRQ 8-15 -> INT 0x28-0x2F
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    port_byte_out(0x21, 0x00);
    port_byte_out(0xA1, 0x00);

    // IRQ1 = keyboard = INT 0x21
    idt_set_gate(0x21, (uint32_t)irq1_handler_asm, 0x08, 0x8E);

    // Mask all IRQs except IRQ1
    port_byte_out(0x21, 0xFD); // 11111101 - only IRQ1 enabled
    port_byte_out(0xA1, 0xFF);

    // Load IDT
    __asm__ volatile("lidt %0" : : "m"(idtp));
    __asm__ volatile("sti");
}