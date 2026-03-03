#ifndef STRING_H
#define STRING_H

#include "kernel.h"

int strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int n);
char* strcat(char* dest, const char* src);
void memset(void* ptr, uint8_t value, size_t num);
void memcpy(void* dest, const void* src, size_t num);
int strstr_index(const char* haystack, const char* needle);
char* strchr(const char* str, int c);
int atoi(const char* str);
void itoa(int value, char* str, int base);
void to_lower(char* str);
int starts_with(const char* str, const char* prefix);
void trim(char* str);

#endif