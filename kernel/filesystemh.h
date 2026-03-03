#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "kernel.h"

#define FS_MAX_FILES 128
#define FS_MAX_NAME 64
#define FS_MAX_PATH 256
#define FS_MAX_CONTENT 4096

#define FS_TYPE_FILE 0
#define FS_TYPE_FOLDER 1

struct fs_entry {
    char name[FS_MAX_NAME];
    char path[FS_MAX_PATH]; // parent path
    int type; // 0 = file, 1 = folder
    char content[FS_MAX_CONTENT];
    int content_len;
    int used;
};

void fs_init(void);
int fs_create(const char* path, const char* name, int type);
int fs_delete(const char* path, const char* name, int type);
int fs_list(const char* path, char* output, int output_size);
int fs_read_file(const char* path, const char* name, char* output, int output_size);
int fs_write_file(const char* path, const char* name, const char* content);
int fs_exists(const char* path, const char* name, int type);
int fs_is_folder(const char* path, const char* name);

extern char current_path[FS_MAX_PATH];

void fs_go(const char* folder_name);
void fs_back(void);
const char* fs_get_current_path(void);

#endif