#include "vga.h"
#include "ports.h"
#include "string.h"

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_color = 0x07; // light grey on black

void vga_init(void) {
    vga_buffer = (uint16_t*)VGA_MEMORY;
    current_color = 0x07;
    cursor_row = 0;
    cursor_col = 0;
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (uint16_t)(' ') | ((uint16_t)current_color << 8);
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_update_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = (bg << 4) | (fg & 0x0F);
}

uint8_t vga_get_fg_color(void) {
    return current_color & 0x0F;
}

void vga_scroll(void) {
    if (cursor_row >= VGA_HEIGHT) {
        // Move everything up one line
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
        }
        // Clear last line
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            vga_buffer[i] = (uint16_t)(' ') | ((uint16_t)current_color << 8);
        }
        cursor_row = VGA_HEIGHT - 1;
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        vga_scroll();
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~3;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
            vga_scroll();
        }
    } else if (c == '\b') {
        vga_backspace();
        return;
    } else {
        int offset = cursor_row * VGA_WIDTH + cursor_col;
        vga_buffer[offset] = (uint16_t)c | ((uint16_t)current_color << 8);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
            vga_scroll();
        }
    }
    vga_update_cursor();
}

void vga_print(const char* str) {
    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

void vga_print_line(const char* str) {
    vga_print(str);
    vga_putchar('\n');
}

void vga_update_cursor(void) {
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;
    port_byte_out(0x3D4, 0x0F);
    port_byte_out(0x3D5, (uint8_t)(pos & 0xFF));
    port_byte_out(0x3D4, 0x0E);
    port_byte_out(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_set_cursor_pos(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    vga_update_cursor();
}

int vga_get_row(void) { return cursor_row; }
int vga_get_col(void) { return cursor_col; }

void vga_backspace(void) {
    if (cursor_col > 0) {
        cursor_col--;
    } else if (cursor_row > 0) {
        cursor_row--;
        cursor_col = VGA_WIDTH - 1;
    }
    int offset = cursor_row * VGA_WIDTH + cursor_col;
    vga_buffer[offset] = (uint16_t)(' ') | ((uint16_t)current_color << 8);
    vga_update_cursor();
}

// Approximate RGB to VGA 16-color
uint8_t rgb_to_vga(int r, int g, int b) {
    // Simple mapping
    int brightness = (r + g + b) / 3;

    if (brightness < 32) return VGA_COLOR_BLACK;

    int max_c = r;
    if (g > max_c) max_c = g;
    if (b > max_c) max_c = b;

    int is_bright = (brightness > 128) ? 1 : 0;

    if (max_c - brightness < 30 && brightness < 100) return VGA_COLOR_DARK_GREY;
    if (max_c - brightness < 30 && brightness >= 100 && brightness < 200) return VGA_COLOR_LIGHT_GREY;
    if (max_c - brightness < 30 && brightness >= 200) return VGA_COLOR_WHITE;

    if (r > g && r > b) {
        return is_bright ? VGA_COLOR_LIGHT_RED : VGA_COLOR_RED;
    }
    if (g > r && g > b) {
        return is_bright ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_GREEN;
    }
    if (b > r && b > g) {
        return is_bright ? VGA_COLOR_LIGHT_BLUE : VGA_COLOR_BLUE;
    }
    if (r > 128 && g > 128 && b < 64) {
        return is_bright ? VGA_COLOR_YELLOW : VGA_COLOR_BROWN;
    }
    if (r > 128 && b > 128 && g < 64) {
        return is_bright ? VGA_COLOR_LIGHT_MAGENTA : VGA_COLOR_MAGENTA;
    }
    if (g > 128 && b > 128 && r < 64) {
        return is_bright ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_CYAN;
    }

    return VGA_COLOR_LIGHT_GREY;
}