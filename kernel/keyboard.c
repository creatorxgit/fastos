#include "keyboard.h"
#include "ports.h"
#include "vga.h"

static char key_buffer[KEY_BUFFER_SIZE];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;
volatile int shift_pressed = 0;
volatile int ctrl_pressed = 0;
volatile int escape_pressed = 0;

// US keyboard layout scancode to ASCII
static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

static const char scancode_to_ascii_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

void keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = 0;
    ctrl_pressed = 0;
    escape_pressed = 0;
}

void keyboard_irq_handler(void) {
    uint8_t scancode = port_byte_in(0x60);

    // Handle shift
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = 1;
        port_byte_out(0x20, 0x20);
        return;
    }
    if (scancode == KEY_LSHIFT_REL || scancode == KEY_RSHIFT_REL) {
        shift_pressed = 0;
        port_byte_out(0x20, 0x20);
        return;
    }

    // Handle ctrl
    if (scancode == KEY_CTRL) {
        ctrl_pressed = 1;
        port_byte_out(0x20, 0x20);
        return;
    }
    if (scancode == KEY_CTRL_REL) {
        ctrl_pressed = 0;
        port_byte_out(0x20, 0x20);
        return;
    }

    // Handle escape
    if (scancode == KEY_ESCAPE) {
        escape_pressed = 1;
        port_byte_out(0x20, 0x20);
        return;
    }

    // Only process key press (not release)
    if (scancode & 0x80) {
        port_byte_out(0x20, 0x20);
        return;
    }

    char c = 0;
    if (scancode < sizeof(scancode_to_ascii)) {
        if (shift_pressed)
            c = scancode_to_ascii_shift[scancode];
        else
            c = scancode_to_ascii[scancode];
    }

    if (c != 0) {
        int next = (buffer_head + 1) % KEY_BUFFER_SIZE;
        if (next != buffer_tail) {
            key_buffer[buffer_head] = c;
            buffer_head = next;
        }
    }

    // Send EOI
    port_byte_out(0x20, 0x20);
}

int keyboard_has_char(void) {
    return buffer_head != buffer_tail;
}

char keyboard_get_char(void) {
    if (buffer_head == buffer_tail) return 0;
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

char keyboard_read_char(void) {
    while (!keyboard_has_char()) {
        __asm__ volatile("hlt");
    }
    return keyboard_get_char();
}

void keyboard_read_line(char* buffer, int max_len, int prompt_col) {
    int pos = 0;
    buffer[0] = 0;

    while (1) {
        char c = keyboard_read_char();

        if (c == '\n') {
            buffer[pos] = 0;
            vga_putchar('\n');
            return;
        } else if (c == '\b') {
            if (pos > 0) {
                pos--;
                buffer[pos] = 0;
                vga_putchar('\b');
            }
        } else if (c >= 32 && pos < max_len - 1) {
            buffer[pos++] = c;
            buffer[pos] = 0;
            vga_putchar(c);
        }
    }
}