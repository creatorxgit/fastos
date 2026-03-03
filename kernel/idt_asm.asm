[BITS 32]
[GLOBAL irq1_handler_asm]
[EXTERN keyboard_irq_handler]

irq1_handler_asm:
    pushad
    call keyboard_irq_handler
    popad
    iret