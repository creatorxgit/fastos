; boot/stage2.asm - Stage 2 Bootloader
; Loaded at 0x7E00 by Stage 1
; Tasks:
;   1. Detect available memory
;   2. Enable A20 line
;   3. Load kernel from disk to 0x8000
;   4. Setup GDT
;   5. Switch to 32-bit protected mode
;   6. Jump to kernel

[BITS 16]
[ORG 0x7E00]

stage2_start:
    mov [boot_drive], dl

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov si, msg_stage2
    call print16

    ; Detect memory
    mov si, msg_memory
    call print16
    call detect_memory

    ; Enable A20
    mov si, msg_a20
    call print16
    call enable_a20
    call check_a20
    cmp ax, 1
    je .a20_ok

    call enable_a20_bios
    call check_a20
    cmp ax, 1
    je .a20_ok

    call enable_a20_keyboard
    call check_a20
    cmp ax, 1
    je .a20_ok

    mov si, msg_a20_fail
    call print16
    jmp $

.a20_ok:
    mov si, msg_a20_ok
    call print16

    ; Load kernel from disk
    mov si, msg_kernel
    call print16

    ; Batch 1: sectors 6-25 -> 0x8000
    mov ax, 20
    mov bx, 0x8000
    mov cx, 6
    call load_sectors

    ; Batch 2: sectors 26-45 -> 0xA800
    mov ax, 20
    mov bx, 0xA800
    mov cx, 26
    call load_sectors

    ; Batch 3: sectors 46-65 -> 0xD000
    mov ax, 20
    mov bx, 0xD000
    mov cx, 46
    call load_sectors

    ; Batch 4: sectors 66-85 -> 0xF800
    mov ax, 20
    mov bx, 0xF800
    mov cx, 66
    call load_sectors

    mov si, msg_kernel_ok
    call print16

    ; Set VGA 80x25 text mode
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; Cursor shape
    mov ah, 0x01
    mov ch, 0x00
    mov cl, 0x0F
    int 0x10

    ; Switch to protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_entry


; =============================================
; 16-bit helper functions
; =============================================

print16:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x07
    int 0x10
    jmp .loop
.done:
    popa
    ret

print_hex_byte:
    pusha
    mov cl, al
    shr al, 4
    call .print_nibble
    mov al, cl
    and al, 0x0F
    call .print_nibble
    popa
    ret
.print_nibble:
    cmp al, 10
    jl .digit
    add al, 'A' - 10
    jmp .print_it
.digit:
    add al, '0'
.print_it:
    mov ah, 0x0E
    int 0x10
    ret

; Load sectors from disk
; AX = count, BX = destination, CX = start sector (LBA)
load_sectors:
    pusha
    mov [.num], ax
    mov [.dest], bx
    mov [.start], cx

    mov ax, [.start]
    xor dx, dx
    mov bx, 18
    div bx
    mov cl, dl
    inc cl

    xor dx, dx
    mov bx, 2
    div bx
    mov ch, al
    mov dh, dl

    mov dl, [boot_drive]
    mov bx, [.dest]
    mov al, [.num]

    mov ah, 0x02
    int 0x13
    jc .error

    popa
    ret

.error:
    mov si, msg_disk_err
    call print16
    mov al, ah
    call print_hex_byte
    mov si, msg_newline
    call print16
    popa
    ret

.num:   dw 0
.dest:  dw 0
.start: dw 0

enable_a20:
    in al, 0x92
    test al, 2
    jnz .done
    or al, 2
    and al, 0xFE
    out 0x92, al
.done:
    ret

enable_a20_bios:
    mov ax, 0x2401
    int 0x15
    ret

enable_a20_keyboard:
    call .wait_input
    mov al, 0xAD
    out 0x64, al
    call .wait_input
    mov al, 0xD0
    out 0x64, al
    call .wait_output
    in al, 0x60
    push ax
    call .wait_input
    mov al, 0xD1
    out 0x64, al
    call .wait_input
    pop ax
    or al, 2
    out 0x60, al
    call .wait_input
    mov al, 0xAE
    out 0x64, al
    call .wait_input
    ret
.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret
.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

check_a20:
    pushf
    push ds
    push es
    push di
    push si

    xor ax, ax
    mov es, ax
    mov di, 0x0500

    mov ax, 0xFFFF
    mov ds, ax
    mov si, 0x0510

    mov al, [es:di]
    push ax
    mov al, [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov [ds:si], al
    pop ax
    mov [es:di], al

    mov ax, 0
    je .disabled
    mov ax, 1
.disabled:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

detect_memory:
    pusha
    mov di, 0x5000
    xor ebx, ebx
    xor bp, bp
    mov edx, 0x534D4150

.e820_loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    mov eax, [di + 8]
    or eax, [di + 12]
    jz .skip
    inc bp
    add di, 24
.skip:
    test ebx, ebx
    jnz .e820_loop
.e820_done:
    mov [0x4FFE], bp
    popa
    ret


; =============================================
; GDT
; =============================================
gdt_start:

gdt_null:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

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

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start


; =============================================
; Data
; =============================================
boot_drive:    db 0

msg_stage2:    db '[Stage2] FastOS Stage 2 Bootloader', 13, 10, 0
msg_memory:    db '[Stage2] Detecting memory...', 13, 10, 0
msg_a20:       db '[Stage2] Enabling A20 line...', 13, 10, 0
msg_a20_ok:    db '[Stage2] A20 enabled.', 13, 10, 0
msg_a20_fail:  db '[Stage2] A20 FAILED!', 13, 10, 0
msg_kernel:    db '[Stage2] Loading kernel...', 13, 10, 0
msg_kernel_ok: db '[Stage2] Kernel loaded.', 13, 10, 0
msg_disk_err:  db '[Stage2] Disk error: 0x', 0
msg_newline:   db 13, 10, 0


; =============================================
; 32-bit Protected Mode Entry
; =============================================
[BITS 32]

protected_mode_entry:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    mov ebp, esp

    ; Clear screen
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0720
    rep stosw

    ; Print boot message
    mov esi, pm_msg
    mov edi, 0xB8000
    mov ah, 0x0A
.print_pm:
    lodsb
    or al, al
    jz .jump_kernel
    stosw
    jmp .print_pm

.jump_kernel:
    ; Small delay
    mov ecx, 0x01FFFFFF
.delay:
    dec ecx
    jnz .delay

    ; Jump to kernel
    jmp 0x8000

    cli
    hlt

pm_msg: db 'FastOS: Booting kernel...', 0


; Pad to 4 sectors (2048 bytes)
times 2048 - ($ - $$) db 0
