#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kernel.h"

#define KEY_BUFFER_SIZE 256

void keyboard_init(void);
void keyboard_irq_handler(void);
char keyboard_get_char(void);
int keyboard_has_char(void);
char keyboard_read_char(void);  // blocking read
void keyboard_read_line(char* buffer, int max_len, int prompt_col);

// Special keys
#define KEY_ENTER 0x1C
#define KEY_BACKSPACE 0x0E
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LSHIFT_REL 0xAA
#define KEY_RSHIFT_REL 0xB6
#define KEY_ESCAPE 0x01
#define KEY_TAB 0x0F
#define KEY_CTRL 0x1D
#define KEY_CTRL_REL 0x9D
#define KEY_S 0x1F

extern volatile int shift_pressed;
extern volatile int ctrl_pressed;
extern volatile int escape_pressed;

#endif