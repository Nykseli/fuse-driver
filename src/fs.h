#ifndef FS_H
#define FS_H

#include "sc_map.h"
#include "util.h"

typedef struct fs_dir {
    const char* name;
    size_t name_len;
    // files and folders
    // TODO: files and folder should probably be in separate maps for faster lookups
    struct sc_map_sv dirs;
    struct sc_map_sv files;
    // TODO: stat
} fs_dir;

typedef struct fs_file {
    uint8_t* data;
    size_t size;
    const char* name;
    size_t name_len;
    // TODO: stat
} fs_file;

/* FROM LSYSFS */
extern int curr_dir_idx;
extern int curr_file_idx;
extern int curr_file_content_idx;
extern fs_dir root_dir;

fs_dir* get_root_dir();

int fs_get_directory(const char* path, fs_dir** buf);
int fs_add_dir(const char* dir_name);
bool fs_is_dir(const char* path);
void init_fs();
void free_fs();

#define fs_foreach(dir_file, key, value) sc_map_foreach(dir_file, key, value)
#define fs_foreach_key(dir_file, key) sc_map_foreach_key(dir_file, key)
#define fs_foreach_val(dir_file, val) sc_map_foreach_value(dir_file, val)

#endif
