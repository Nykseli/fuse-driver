#ifndef FS_H
#define FS_H

#include <sys/types.h>

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

typedef struct path_string {
    // pointer to the original from fuse funtions
    const char* path;

    // copy of the path containting the divided paths
    char path_copy[PATH_LEN_MAX];
    // indexes of the paths
    int idx_buff[256];
    // count of the indexes, if 0. this is the root file
    int files;
} path_string;

int create_path_string(path_string* p_string, const char* path);

int fs_get_file(path_string* p_string, fs_file** buf);
int fs_get_directory(path_string* p_string, fs_dir** buf, int offset);
int fs_add_dir_or_file(path_string* p_string, bool is_dir);
bool fs_is_dir(path_string* p_string);
bool fs_is_file(path_string* p_string);
int fs_file_read(path_string* p_string, char* buffer, size_t size, off_t offset);
int fs_file_write(path_string* p_string, const char* buffer, size_t size, off_t offset);
int fs_file_truncate(path_string* p_string, off_t size);
void init_fs();
void free_fs();

#define fs_foreach(dir_file, key, value) sc_map_foreach(dir_file, key, value)
#define fs_foreach_key(dir_file, key) sc_map_foreach_key(dir_file, key)
#define fs_foreach_val(dir_file, val) sc_map_foreach_value(dir_file, val)

#endif
