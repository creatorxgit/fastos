; boot/boot.asm - Stage 1 Bootloader (MBR)
[BITS 16]
[ORG 0x7C00]

start:
    ; Setup segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Save boot drive
    mov [boot_drive], dl

    ; Clear screen
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Print loading message
    mov si, msg_loading
    call print_string_16

    ; Load stage 2 + kernel from disk
    ; Load 60 sectors starting from sector 2 to 0x8000
    mov ah, 0x02        ; BIOS read sectors
    mov al, 60           ; number of sectors to read
    mov ch, 0            ; cylinder 0
    mov cl, 2            ; start from sector 2
    mov dh, 0            ; head 0
    mov dl, [boot_drive] ; drive number
    mov bx, 0x8000       ; load to 0x0000:0x8000
    int 0x13
    jc disk_error

    ; Enable A20 line
    call enable_a20

    ; Load GDT
    cli
    lgdt [gdt_descriptor]

    ; Switch to protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code
    jmp 0x08:protected_mode

disk_error:
    mov si, msg_disk_error
    call print_string_16
    jmp $

print_string_16:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string_16
.done:
    ret

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; GDT
gdt_start:
    ; Null descriptor
    dq 0

gdt_code:
    dw 0xFFFF    ; limit low
    dw 0x0000    ; base low
    db 0x00      ; base middle
    db 10011010b ; access
    db 11001111b ; granularity
    db 0x00      ; base high

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_mode:
    ; Setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Jump to kernel at 0x8000
    jmp 0x8000

boot_drive: db 0
msg_loading: db 'FastOS Loading...', 13, 10, 0
msg_disk_error: db 'Disk Error!', 0

; Pad to 510 bytes and add boot signature
times 510 - ($ - $$) db 0
dw 0xAA55