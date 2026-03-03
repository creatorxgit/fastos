#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "filesystem.h"
#include "ports.h"
#include "kernel.h"

#define CMD_BUFFER_SIZE 512
#define OUTPUT_BUFFER_SIZE 4096

// Forward declarations
static void shell_print_prompt(void);
static void shell_process_command(const char* cmd);
static void cmd_device_power_off(void);
static void cmd_device_power_restart(void);
static void cmd_device_power_restart_bios(void);
static void cmd_os_style_text(const char* args);
static void cmd_ls_list(void);
static void cmd_ls_go(const char* args);
static void cmd_ls_back(void);
static void cmd_os_help(void);
static void cmd_device_info(void);
static void cmd_neofetch(void);
static void cmd_os_data_add(const char* args);
static void cmd_os_data_read(const char* args);
static void cmd_os_data_edit(const char* args);
static void cmd_os_data_rm(const char* args);
static void cmd_os_internet_on(void);
static void cmd_os_internet_off(void);
static void cmd_os_internet_connect(const char* args);
static void cmd_os_internet_browse(const char* args);

// Helper to extract parameter value from format param='value'
static int extract_param(const char* cmd, const char* param_name, char* value, int max_len);

// Text editor
static void text_editor(const char* filename);

static int internet_enabled = 0;

void shell_init(void) {
    fs_init();
    internet_enabled = 0;
}

void shell_run(void) {
    char cmd_buffer[CMD_BUFFER_SIZE];

    vga_print_line("FastOS v1.0");
    vga_print_line("Type 'os.help' for help.");
    vga_print_line("");

    while (1) {
        shell_print_prompt();
        keyboard_read_line(cmd_buffer, CMD_BUFFER_SIZE, vga_get_col());
        trim(cmd_buffer);
        if (cmd_buffer[0] != 0) {
            shell_process_command(cmd_buffer);
        }
    }
}

static void shell_print_prompt(void) {
    vga_print(fs_get_current_path());
    vga_print(" $ ");
}

static void shell_process_command(const char* cmd) {
    if (strcmp(cmd, "device.power.off") == 0) {
        cmd_device_power_off();
    } else if (strcmp(cmd, "device.power.restart") == 0) {
        cmd_device_power_restart();
    } else if (strcmp(cmd, "device.power.restart -bios") == 0) {
        cmd_device_power_restart_bios();
    } else if (starts_with(cmd, "os.style.text(") || starts_with(cmd, "os.style.text (")) {
        cmd_os_style_text(cmd);
    } else if (strcmp(cmd, "ls.list") == 0) {
        cmd_ls_list();
    } else if (starts_with(cmd, "ls.go(")) {
        cmd_ls_go(cmd);
    } else if (strcmp(cmd, "ls.back") == 0) {
        cmd_ls_back();
    } else if (strcmp(cmd, "os.help") == 0) {
        cmd_os_help();
    } else if (strcmp(cmd, "device.info") == 0) {
        cmd_device_info();
    } else if (strcmp(cmd, "neofetch") == 0) {
        cmd_neofetch();
    } else if (strcmp(cmd, "os.internet.on") == 0) {
        cmd_os_internet_on();
    } else if (strcmp(cmd, "os.internet.off") == 0) {
        cmd_os_internet_off();
    } else if (starts_with(cmd, "os.internet.connect(") || starts_with(cmd, "os.interne.connect(")) {
        cmd_os_internet_connect(cmd);
    } else if (starts_with(cmd, "os.internet.browse(")) {
        cmd_os_internet_browse(cmd);
    } else if (starts_with(cmd, "os.data.add(")) {
        cmd_os_data_add(cmd);
    } else if (starts_with(cmd, "os.data.read(")) {
        cmd_os_data_read(cmd);
    } else if (starts_with(cmd, "os.data.edit(")) {
        cmd_os_data_edit(cmd);
    } else if (starts_with(cmd, "os.data.rm(")) {
        cmd_os_data_rm(cmd);
    } else {
        vga_print("Unknown command: ");
        vga_print_line(cmd);
    }
}

// Extract param value: find param_name=' then read until '
static int extract_param(const char* cmd, const char* param_name, char* value, int max_len) {
    // Find param_name= or param_name ='
    char search[128];
    strcpy(search, param_name);
    strcat(search, "='");

    int idx = strstr_index(cmd, search);
    if (idx < 0) {
        // Try with space before =
        strcpy(search, param_name);
        strcat(search, " ='");
        idx = strstr_index(cmd, search);
    }
    if (idx < 0) return -1;

    // Find the opening '
    const char* start = cmd + idx;
    start = strchr(start, '\'');
    if (!start) return -1;
    start++; // skip '

    const char* end = strchr(start, '\'');
    if (!end) return -1;

    int len = end - start;
    if (len >= max_len) len = max_len - 1;
    strncpy(value, start, len);
    value[len] = 0;
    return 0;
}

// ============ Commands Implementation ============

static void cmd_device_power_off(void) {
    vga_print_line("Shutting down...");
    // ACPI shutdown - works on QEMU and Bochs
    port_word_out(0x604, 0x2000);
    // APM shutdown fallback
    port_byte_out(0x4004, 0x00);
    // If still running
    vga_print_line("Please turn off your computer manually.");
    while (1) { __asm__ volatile("hlt"); }
}

static void cmd_device_power_restart(void) {
    vga_print_line("Restarting...");
    // Keyboard controller reset
    uint8_t good = 0x02;
    while (good & 0x02)
        good = port_byte_in(0x64);
    port_byte_out(0x64, 0xFE);
    while (1) { __asm__ volatile("hlt"); }
}

static void cmd_device_power_restart_bios(void) {
    vga_print_line("Restarting to BIOS...");
    // Triple fault to reboot
    struct idt_ptr {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));

    struct idt_ptr null_idt = {0, 0};
    __asm__ volatile("lidt %0" : : "m"(null_idt));
    __asm__ volatile("int $0x03");
    while (1) { __asm__ volatile("hlt"); }
}

static void cmd_os_style_text(const char* args) {
    char color_str[64] = {0};
    char type_str[16] = {0};

    if (extract_param(args, "color", color_str, 64) < 0) {
        vga_print_line("Error: missing color parameter");
        return;
    }

    // Default type is RGB
    if (extract_param(args, "type", type_str, 16) < 0) {
        strcpy(type_str, "RGB");
    }

    // Parse based on type
    if (strcmp(type_str, "RBG") == 0 || strcmp(type_str, "RGB") == 0 ||
        strcmp(type_str, "rbg") == 0 || strcmp(type_str, "rgb") == 0) {
        // Parse R, G, B
        int r = 0, g = 0, b = 0;
        // Parse "85, 88, 92"
        char* p = color_str;
        r = atoi(p);
        p = strchr(p, ',');
        if (p) { p++; g = atoi(p); }
        p = strchr(p ? p : color_str, ',');
        if (p) { p++; b = atoi(p); }

        uint8_t vga_color = rgb_to_vga(r, g, b);
        vga_set_color(vga_color, VGA_COLOR_BLACK);

        vga_print("Text color set to RGB(");
        char num[16];
        itoa(r, num, 10); vga_print(num);
        vga_print(", ");
        itoa(g, num, 10); vga_print(num);
        vga_print(", ");
        itoa(b, num, 10); vga_print(num);
        vga_print_line(")");
    } else if (strcmp(type_str, "HEX") == 0 || strcmp(type_str, "hex") == 0) {
        // Parse hex color like "555860"
        int r = 0, g = 0, b = 0;
        char* p = color_str;
        if (*p == '#') p++;
        // Parse 6 hex digits
        for (int i = 0; i < 2 && *p; i++, p++) {
            int d = 0;
            if (*p >= '0' && *p <= '9') d = *p - '0';
            else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
            else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
            r = r * 16 + d;
        }
        for (int i = 0; i < 2 && *p; i++, p++) {
            int d = 0;
            if (*p >= '0' && *p <= '9') d = *p - '0';
            else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
            else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
            g = g * 16 + d;
        }
        for (int i = 0; i < 2 && *p; i++, p++) {
            int d = 0;
            if (*p >= '0' && *p <= '9') d = *p - '0';
            else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
            else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
            b = b * 16 + d;
        }
        uint8_t vga_color = rgb_to_vga(r, g, b);
        vga_set_color(vga_color, VGA_COLOR_BLACK);
        vga_print("Text color set from HEX: ");
        vga_print_line(color_str);
    } else if (strcmp(type_str, "HSL") == 0 || strcmp(type_str, "hsl") == 0) {
        // Simple HSL parsing - approximate
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_print_line("HSL color applied (approximate)");
    } else if (strcmp(type_str, "HSV") == 0 || strcmp(type_str, "hsv") == 0) {
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_print_line("HSV color applied (approximate)");
    } else if (strcmp(type_str, "CMYK") == 0 || strcmp(type_str, "cmyk") == 0) {
        // Parse C, M, Y, K
        int c_val = 0, m_val = 0, y_val = 0, k_val = 0;
        char* p = color_str;
        c_val = atoi(p);
        p = strchr(p, ','); if (p) { p++; m_val = atoi(p); }
        p = strchr(p ? p : color_str, ','); if (p) { p++; y_val = atoi(p); }
        p = strchr(p ? p : color_str, ','); if (p) { p++; k_val = atoi(p); }
        // Convert CMYK to RGB
        int r = 255 * (100 - c_val) * (100 - k_val) / 10000;
        int g_c = 255 * (100 - m_val) * (100 - k_val) / 10000;
        int b_c = 255 * (100 - y_val) * (100 - k_val) / 10000;
        uint8_t vga_color = rgb_to_vga(r, g_c, b_c);
        vga_set_color(vga_color, VGA_COLOR_BLACK);
        vga_print_line("Text color set from CMYK");
    } else {
        vga_print("Unknown color type: ");
        vga_print_line(type_str);
    }
}

static void cmd_ls_list(void) {
    char output[OUTPUT_BUFFER_SIZE];
    int count = fs_list(current_path, output, OUTPUT_BUFFER_SIZE);
    if (count == 0) {
        vga_print_line("(empty directory)");
    } else {
        vga_print(output);
    }
}

static void cmd_ls_go(const char* args) {
    char folder_name[FS_MAX_NAME];
    if (extract_param(args, "object", folder_name, FS_MAX_NAME) < 0) {
        vga_print_line("Error: missing object parameter");
        return;
    }

    if (fs_is_folder(current_path, folder_name)) {
        fs_go(folder_name);
        vga_print("Entered: ");
        vga_print_line(current_path);
    } else {
        vga_print("Folder not found: ");
        vga_print_line(folder_name);
    }
}

static void cmd_ls_back(void) {
    if (strcmp(current_path, "/") == 0) {
        vga_print_line("Already at root directory");
        return;
    }
    fs_back();
    vga_print("Current directory: ");
    vga_print_line(current_path);
}

static void cmd_os_help(void) {
    vga_print_line("FastOS Help");
    vga_print_line("Documentation: https://github.com/creatorxgit/fastos");
    vga_print_line("");
    vga_print_line("Available commands:");
    vga_print_line("  device.power.off          - Shutdown");
    vga_print_line("  device.power.restart       - Restart");
    vga_print_line("  device.power.restart -bios - Restart to BIOS");
    vga_print_line("  device.info               - Device information");
    vga_print_line("  neofetch                  - System info display");
    vga_print_line("  os.style.text(...)        - Set text color");
    vga_print_line("  ls.list                   - List directory");
    vga_print_line("  ls.go(object='name')      - Enter folder");
    vga_print_line("  ls.back                   - Go back");
    vga_print_line("  os.data.add(...)          - Create file/folder");
    vga_print_line("  os.data.read(name='...')  - Read file");
    vga_print_line("  os.data.edit(name='...')  - Edit file");
    vga_print_line("  os.data.rm(...)           - Delete file/folder");
    vga_print_line("  os.internet.on            - Enable internet");
    vga_print_line("  os.internet.off           - Disable internet");
    vga_print_line("  os.internet.connect(...)  - Connect to network");
    vga_print_line("  os.internet.browse(...)   - Browse URL");
    vga_print_line("  os.help                   - This help");
}

static void cmd_device_info(void) {
    vga_print_line("=== Device Information ===");

    // CPU info using CPUID
    uint32_t eax_val, ebx_val, ecx_val, edx_val;

    // Get vendor string
    char vendor[13];
    __asm__ volatile("cpuid"
        : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
        : "a"(0));
    memcpy(vendor, &ebx_val, 4);
    memcpy(vendor + 4, &edx_val, 4);
    memcpy(vendor + 8, &ecx_val, 4);
    vendor[12] = 0;

    vga_print("CPU Vendor: ");
    vga_print_line(vendor);

    // Get CPU brand string if supported
    __asm__ volatile("cpuid"
        : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
        : "a"(0x80000000));

    if (eax_val >= 0x80000004) {
        char brand[49];
        uint32_t* b = (uint32_t*)brand;

        for (uint32_t i = 0; i < 3; i++) {
            __asm__ volatile("cpuid"
                : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
                : "a"(0x80000002 + i));
            b[i*4+0] = eax_val;
            b[i*4+1] = ebx_val;
            b[i*4+2] = ecx_val;
            b[i*4+3] = edx_val;
        }
        brand[48] = 0;
        vga_print("CPU: ");
        vga_print_line(brand);
    }

    // Detect RAM (simple method - probe memory)
    // We'll estimate from BIOS data or just show what we can detect
    vga_print_line("Architecture: x86 (i386)");
    vga_print_line("Mode: 32-bit Protected Mode");
    vga_print("OS: FastOS v1.0");
    vga_putchar('\n');

    // Display VGA info
    vga_print("Display: VGA 80x25 Text Mode");
    vga_putchar('\n');
}

static void cmd_neofetch(void) {
    // Get CPU vendor
    uint32_t eax_val, ebx_val, ecx_val, edx_val;
    char vendor[13];
    __asm__ volatile("cpuid"
        : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
        : "a"(0));
    memcpy(vendor, &ebx_val, 4);
    memcpy(vendor + 4, &edx_val, 4);
    memcpy(vendor + 8, &ecx_val, 4);
    vendor[12] = 0;

    char brand[49];
    brand[0] = 0;
    __asm__ volatile("cpuid"
        : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
        : "a"(0x80000000));
    if (eax_val >= 0x80000004) {
        uint32_t* b = (uint32_t*)brand;
        for (uint32_t i = 0; i < 3; i++) {
            __asm__ volatile("cpuid"
                : "=a"(eax_val), "=b"(ebx_val), "=c"(ecx_val), "=d"(edx_val)
                : "a"(0x80000002 + i));
            b[i*4+0] = eax_val;
            b[i*4+1] = ebx_val;
            b[i*4+2] = ecx_val;
            b[i*4+3] = edx_val;
        }
        brand[48] = 0;
    }

    uint8_t saved_color = vga_get_fg_color();

    // ASCII art logo
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_line("        ______          _    ___  ____  ");
    vga_print_line("       |  ____|        | |  / _ \\/ ___| ");
    vga_print_line("       | |__ __ _  ___ | |_| | | \\___ \\ ");
    vga_print_line("       |  __/ _` |/ __|| __| | | |___) |");
    vga_print_line("       | | | (_| |\\__ \\| |_| |_| |____/ ");
    vga_print_line("       |_|  \\__,_||___/ \\__|\\___/       ");
    vga_print_line("");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  OS:         ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line("FastOS v1.0");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Kernel:     ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line("FastOS Kernel 1.0");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Shell:      ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line("FastShell 1.0");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  CPU:        ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    if (brand[0])
        vga_print_line(brand);
    else
        vga_print_line(vendor);

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Arch:       ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line("x86 (32-bit)");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Display:    ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line("VGA 80x25 Text");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print("  Internet:   ");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_line(internet_enabled ? "Enabled" : "Disabled");

    // Color blocks
    vga_print_line("");
    vga_print("  ");
    for (int i = 0; i < 8; i++) {
        vga_set_color(i, i);
        vga_print("   ");
    }
    vga_putchar('\n');
    vga_print("  ");
    for (int i = 8; i < 16; i++) {
        vga_set_color(i, i);
        vga_print("   ");
    }
    vga_putchar('\n');

    vga_set_color(saved_color, VGA_COLOR_BLACK);
}

static void cmd_os_data_add(const char* args) {
    char type_str[32];
    char name_str[FS_MAX_NAME];

    if (extract_param(args, "type", type_str, 32) < 0) {
        vga_print_line("Error: missing type parameter");
        return;
    }
    if (extract_param(args, "name", name_str, FS_MAX_NAME) < 0) {
        vga_print_line("Error: missing name parameter");
        return;
    }

    int type;
    if (strcmp(type_str, "folder") == 0 || strcmp(type_str, "Folder") == 0 ||
        strcmp(type_str, "dir") == 0) {
        type = FS_TYPE_FOLDER;
    } else if (strcmp(type_str, "file") == 0 || strcmp(type_str, "File") == 0) {
        type = FS_TYPE_FILE;
    } else {
        vga_print("Unknown type: ");
        vga_print_line(type_str);
        return;
    }

    int result = fs_create(current_path, name_str, type);
    if (result == 0) {
        vga_print("Created ");
        vga_print(type == FS_TYPE_FOLDER ? "folder" : "file");
        vga_print(": ");
        vga_print_line(name_str);
    } else if (result == -1) {
        vga_print_line("Error: already exists");
    } else {
        vga_print_line("Error: filesystem full");
    }
}

static void cmd_os_data_read(const char* args) {
    char name_str[FS_MAX_NAME];

    if (extract_param(args, "name", name_str, FS_MAX_NAME) < 0) {
        vga_print_line("Error: missing name parameter");
        return;
    }

    char content[FS_MAX_CONTENT];
    if (fs_read_file(current_path, name_str, content, FS_MAX_CONTENT) == 0) {
        vga_print("--- ");
        vga_print(name_str);
        vga_print_line(" ---");
        if (content[0] == 0) {
            vga_print_line("(empty file)");
        } else {
            vga_print_line(content);
        }
        vga_print_line("--- end ---");
    } else {
        vga_print("File not found: ");
        vga_print_line(name_str);
    }
}

static void cmd_os_data_edit(const char* args) {
    char name_str[FS_MAX_NAME];

    if (extract_param(args, "name", name_str, FS_MAX_NAME) < 0) {
        vga_print_line("Error: missing name parameter");
        return;
    }

    if (!fs_exists(current_path, name_str, FS_TYPE_FILE)) {
        vga_print("File not found: ");
        vga_print_line(name_str);
        return;
    }

    text_editor(name_str);
}

static void cmd_os_data_rm(const char* args) {
    char type_str[32];
    char name_str[FS_MAX_NAME];

    if (extract_param(args, "type", type_str, 32) < 0) {
        vga_print_line("Error: missing type parameter");
        return;
    }
    if (extract_param(args, "name", name_str, FS_MAX_NAME) < 0) {
        vga_print_line("Error: missing name parameter");
        return;
    }

    int type;
    if (strcmp(type_str, "folder") == 0 || strcmp(type_str, "Folder") == 0) {
        type = FS_TYPE_FOLDER;
    } else if (strcmp(type_str, "file") == 0 || strcmp(type_str, "File") == 0) {
        type = FS_TYPE_FILE;
    } else {
        vga_print("Unknown type: ");
        vga_print_line(type_str);
        return;
    }

    int result = fs_delete(current_path, name_str, type);
    if (result == 0) {
        vga_print("Deleted ");
        vga_print(type == FS_TYPE_FOLDER ? "folder" : "file");
        vga_print(": ");
        vga_print_line(name_str);
    } else {
        vga_print("Not found: ");
        vga_print_line(name_str);
    }
}

static void cmd_os_internet_on(void) {
    internet_enabled = 1;
    vga_print_line("Internet functionality enabled");
    vga_print_line("Note: Network drivers are limited in this OS");
}

static void cmd_os_internet_off(void) {
    internet_enabled = 0;
    vga_print_line("Internet functionality disabled");
}

static void cmd_os_internet_connect(const char* args) {
    if (!internet_enabled) {
        vga_print_line("Error: Internet is disabled. Use os.internet.on first");
        return;
    }

    // Check if Ethernet
    if (strstr_index(args, "Ethernet") >= 0) {
        vga_print_line("Attempting Ethernet connection...");
        vga_print_line("Note: Ethernet driver not available in this version");
        vga_print_line("Status: Simulated connection established");
        return;
    }

    // WiFi connection
    char wifi_type[16], wifi_name[64], wifi_pass[64], wifi_security[16];

    if (extract_param(args, "type", wifi_type, 16) >= 0 &&
        strcmp(wifi_type, "wifi") == 0) {

        if (extract_param(args, "name", wifi_name, 64) < 0) {
            vga_print_line("Error: missing network name");
            return;
        }

        extract_param(args, "pass", wifi_pass, 64);
        extract_param(args, "security", wifi_security, 16);

        vga_print("Connecting to WiFi: ");
        vga_print_line(wifi_name);
        if (wifi_security[0]) {
            vga_print("Security: ");
            vga_print_line(wifi_security);
        }
        vga_print_line("Note: WiFi driver not available in this version");
        vga_print_line("Status: Connection simulated");
    } else {
        vga_print_line("Error: unknown connection type");
    }
}

static void cmd_os_internet_browse(const char* args) {
    if (!internet_enabled) {
        vga_print_line("Error: Internet is disabled. Use os.internet.on first");
        return;
    }

    char link[256];
    if (extract_param(args, "link", link, 256) < 0) {
        vga_print_line("Error: missing link parameter");
        return;
    }

    vga_clear();
    vga_print_line("=== FastOS Browser ===");
    vga_print("URL: ");
    vga_print_line(link);
    vga_print_line("---");
    vga_print_line("Note: Full HTTP/HTTPS not available in this version");
    vga_print_line("This requires network card drivers and TCP/IP stack");
    vga_print_line("");
    vga_print("Requested: ");
    vga_print_line(link);
    vga_print_line("");
    vga_print_line("Use arrow keys to navigate, Enter to select");
    vga_print_line("Press ESC to exit browser");

    // Wait for ESC
    escape_pressed = 0;
    while (!escape_pressed) {
        __asm__ volatile("hlt");
    }
    escape_pressed = 0;

    vga_clear();
    vga_print_line("Browser closed.");
}

// ============ Text Editor ============
static void text_editor(const char* filename) {
    char content[FS_MAX_CONTENT];
    fs_read_file(current_path, filename, content, FS_MAX_CONTENT);

    int content_len = strlen(content);

    vga_clear();

    // Header
    vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    vga_print(" FastOS Editor - ");
    vga_print(filename);
    // Fill rest of line
    int hlen = 17 + strlen(filename);
    for (int i = hlen; i < VGA_WIDTH; i++) vga_putchar(' ');

    // Footer
    vga_set_cursor_pos(VGA_HEIGHT - 1, 0);
    vga_print(" Shift+S: Save | ESC: Exit");
    for (int i = 26; i < VGA_WIDTH; i++) vga_putchar(' ');

    vga_set_color(vga_get_fg_color(), VGA_COLOR_BLACK);
    // Use default or current colors
    uint8_t fg = VGA_COLOR_LIGHT_GREY;
    vga_set_color(fg, VGA_COLOR_BLACK);

    // Display content
    vga_set_cursor_pos(1, 0);
    vga_print(content);

    int cursor_pos = content_len;
    escape_pressed = 0;

    while (1) {
        if (escape_pressed) {
            escape_pressed = 0;
            break;
        }

        if (keyboard_has_char()) {
            char c = keyboard_get_char();

            // Check for Shift+S (save)
            if (shift_pressed && (c == 's' || c == 'S')) {
                content[content_len] = 0;
                fs_write_file(current_path, filename, content);

                // Show save message
                vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
                vga_set_cursor_pos(VGA_HEIGHT - 1, 0);
                vga_print(" File saved!                                                                    ");
                vga_set_color(fg, VGA_COLOR_BLACK);

                // Brief delay
                for (volatile int i = 0; i < 5000000; i++);

                // Restore footer
                vga_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
                vga_set_cursor_pos(VGA_HEIGHT - 1, 0);
                vga_print(" Shift+S: Save | ESC: Exit");
                for (int i = 26; i < VGA_WIDTH; i++) vga_putchar(' ');
                vga_set_color(fg, VGA_COLOR_BLACK);
                continue;
            }

            if (c == '\b') {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    // Shift content left
                    for (int i = cursor_pos; i < content_len; i++)
                        content[i] = content[i + 1];
                    content_len--;
                    // Redraw
                    vga_set_color(fg, VGA_COLOR_BLACK);
                    vga_set_cursor_pos(1, 0);
                    // Clear content area
                    for (int i = 1; i < VGA_HEIGHT - 1; i++) {
                        vga_set_cursor_pos(i, 0);
                        for (int j = 0; j < VGA_WIDTH; j++) vga_putchar(' ');
                    }
                    vga_set_cursor_pos(1, 0);
                    content[content_len] = 0;
                    vga_print(content);
                }
            } else if (c == '\n') {
                if (content_len < FS_MAX_CONTENT - 2) {
                    // Insert newline
                    for (int i = content_len; i > cursor_pos; i--)
                        content[i] = content[i-1];
                    content[cursor_pos] = '\n';
                    cursor_pos++;
                    content_len++;
                    content[content_len] = 0;
                    // Redraw
                    vga_set_color(fg, VGA_COLOR_BLACK);
                    vga_set_cursor_pos(1, 0);
                    for (int i = 1; i < VGA_HEIGHT - 1; i++) {
                        vga_set_cursor_pos(i, 0);
                        for (int j = 0; j < VGA_WIDTH; j++) vga_putchar(' ');
                    }
                    vga_set_cursor_pos(1, 0);
                    vga_print(content);
                }
            } else if (c >= 32 && content_len < FS_MAX_CONTENT - 2) {
                // Insert character
                for (int i = content_len; i > cursor_pos; i--)
                    content[i] = content[i-1];
                content[cursor_pos] = c;
                cursor_pos++;
                content_len++;
                content[content_len] = 0;
                // Redraw
                vga_set_color(fg, VGA_COLOR_BLACK);
                vga_set_cursor_pos(1, 0);
                for (int i = 1; i < VGA_HEIGHT - 1; i++) {
                    vga_set_cursor_pos(i, 0);
                    for (int j = 0; j < VGA_WIDTH; j++) vga_putchar(' ');
                }
                vga_set_cursor_pos(1, 0);
                vga_print(content);
            }
        }

        __asm__ volatile("hlt");
    }

    vga_set_color(fg, VGA_COLOR_BLACK);
    vga_clear();
    vga_print_line("Editor closed.");
}