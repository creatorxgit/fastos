#include "filesystem.h"
#include "string.h"

struct fs_entry fs_entries[FS_MAX_FILES];
char current_path[FS_MAX_PATH];

void fs_init(void) {
    memset(fs_entries, 0, sizeof(fs_entries));
    strcpy(current_path, "/");
}

int fs_create(const char* path, const char* name, int type) {
    // Check if already exists
    if (fs_exists(path, name, type)) return -1;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!fs_entries[i].used) {
            fs_entries[i].used = 1;
            strncpy(fs_entries[i].name, name, FS_MAX_NAME - 1);
            strncpy(fs_entries[i].path, path, FS_MAX_PATH - 1);
            fs_entries[i].type = type;
            fs_entries[i].content[0] = 0;
            fs_entries[i].content_len = 0;
            return 0;
        }
    }
    return -2; // no space
}

int fs_delete(const char* path, const char* name, int type) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_entries[i].used &&
            strcmp(fs_entries[i].name, name) == 0 &&
            strcmp(fs_entries[i].path, path) == 0 &&
            fs_entries[i].type == type) {

            // If folder, delete contents recursively
            if (type == FS_TYPE_FOLDER) {
                char sub_path[FS_MAX_PATH];
                strcpy(sub_path, path);
                if (sub_path[strlen(sub_path)-1] != '/') strcat(sub_path, "/");
                strcat(sub_path, name);

                for (int j = 0; j < FS_MAX_FILES; j++) {
                    if (fs_entries[j].used && strcmp(fs_entries[j].path, sub_path) == 0) {
                        fs_entries[j].used = 0;
                    }
                }
            }

            fs_entries[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int fs_list(const char* path, char* output, int output_size) {
    output[0] = 0;
    int count = 0;

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_entries[i].used && strcmp(fs_entries[i].path, path) == 0) {
            if (strlen(output) + strlen(fs_entries[i].name) + 16 < (uint32_t)output_size) {
                if (fs_entries[i].type == FS_TYPE_FOLDER) {
                    strcat(output, "[DIR]  ");
                } else {
                    strcat(output, "[FILE] ");
                }
                strcat(output, fs_entries[i].name);
                strcat(output, "\n");
                count++;
            }
        }
    }
    return count;
}

int fs_read_file(const char* path, const char* name, char* output, int output_size) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_entries[i].used &&
            strcmp(fs_entries[i].name, name) == 0 &&
            strcmp(fs_entries[i].path, path) == 0 &&
            fs_entries[i].type == FS_TYPE_FILE) {
            strncpy(output, fs_entries[i].content, output_size - 1);
            output[output_size - 1] = 0;
            return 0;
        }
    }
    return -1;
}

int fs_write_file(const char* path, const char* name, const char* content) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_entries[i].used &&
            strcmp(fs_entries[i].name, name) == 0 &&
            strcmp(fs_entries[i].path, path) == 0 &&
            fs_entries[i].type == FS_TYPE_FILE) {
            strncpy(fs_entries[i].content, content, FS_MAX_CONTENT - 1);
            fs_entries[i].content_len = strlen(content);
            return 0;
        }
    }
    return -1;
}

int fs_exists(const char* path, const char* name, int type) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs_entries[i].used &&
            strcmp(fs_entries[i].name, name) == 0 &&
            strcmp(fs_entries[i].path, path) == 0 &&
            fs_entries[i].type == type) {
            return 1;
        }
    }
    return 0;
}

int fs_is_folder(const char* path, const char* name) {
    return fs_exists(path, name, FS_TYPE_FOLDER);
}

void fs_go(const char* folder_name) {
    if (fs_is_folder(current_path, folder_name)) {
        if (current_path[strlen(current_path)-1] != '/')
            strcat(current_path, "/");
        strcat(current_path, folder_name);
    }
}

void fs_back(void) {
    if (strcmp(current_path, "/") == 0) return;

    int len = strlen(current_path);
    // Find last /
    for (int i = len - 1; i >= 0; i--) {
        if (current_path[i] == '/') {
            if (i == 0)
                current_path[1] = 0;
            else
                current_path[i] = 0;
            return;
        }
    }
}

const char* fs_get_current_path(void) {
    return current_path;
}