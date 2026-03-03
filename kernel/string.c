#include "string.h"

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        if (s1[i] == 0) return 0;
    }
    return 0;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n && src[i]; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = 0;
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

void memset(void* ptr, uint8_t value, size_t num) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < num; i++)
        p[i] = value;
}

void memcpy(void* dest, const void* src, size_t num) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < num; i++)
        d[i] = s[i];
}

int strstr_index(const char* haystack, const char* needle) {
    int hlen = strlen(haystack);
    int nlen = strlen(needle);
    for (int i = 0; i <= hlen - nlen; i++) {
        int j;
        for (j = 0; j < nlen; j++) {
            if (haystack[i + j] != needle[j]) break;
        }
        if (j == nlen) return i;
    }
    return -1;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) return (char*)str;
        str++;
    }
    if (c == 0) return (char*)str;
    return NULL;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    while (*str == ' ') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}

void itoa(int value, char* str, int base) {
    char* p = str;
    char* p1, *p2;
    unsigned int ud_value;
    int negative = 0;

    if (base == 10 && value < 0) {
        negative = 1;
        ud_value = (unsigned int)(-value);
    } else {
        ud_value = (unsigned int)value;
    }

    do {
        int remainder = ud_value % base;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud_value /= base);

    if (negative) *p++ = '-';
    *p = 0;

    // Reverse
    p1 = str;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

void to_lower(char* str) {
    while (*str) {
        if (*str >= 'A' && *str <= 'Z')
            *str += 32;
        str++;
    }
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

void trim(char* str) {
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = 0;
    }
    int start = 0;
    while (str[start] == ' ') start++;
    if (start > 0) {
        int i;
        for (i = 0; str[start + i]; i++)
            str[i] = str[start + i];
        str[i] = 0;
    }
}