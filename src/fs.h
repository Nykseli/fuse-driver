#ifndef FS_H
#define FS_H

#include <sys/statvfs.h>
#include <sys/types.h>

#include "sc_map.h"
#include "util.h"

typedef enum FS_ITEM_TYPE {
    FS_DIR,
    FS_FILE
} FS_ITEM_TYPE;

typedef struct fs_dir {
    struct sc_map_sv items;
} fs_dir;

typedef struct fs_file {
    uint8_t* data;
    size_t size;
} fs_file;

typedef struct fs_item {
    const char* name;
    size_t name_len;
    // null if root dir
    // will always point to a fs_dir item
    struct fs_item* parent;
    // TODO: stat and replace type with it
    FS_ITEM_TYPE type;
    union {
        fs_dir dir;
        fs_file file;
    } as;
} fs_item;

typedef struct path_string {
    // pointer to the original from fuse funtions
    const char* path;

    // copy of the path containting the divided paths
    char path_copy[PATH_LEN_MAX];
    // indexes of the paths
    // TODO: collect the length of paths so we do faster map lookups
    //       this requires some changes in sc_map
    int idx_buff[256];
    // count of the indexes, if 0. this is the root file
    int files;
} path_string;

#define fs_item_dir(_item) (_item)->as.dir
#define fs_item_file(_item) (_item)->as.file

int create_path_string(path_string* p_string, const char* path);

// TODO: make fs_ to reflect syscalls. fs_read, fs_unlink etc

int fs_get_file(path_string* p_string, fs_file** buf);
int fs_get_item(path_string* p_string, fs_item** buf, int offset);
int fs_get_directory(path_string* p_string, fs_dir** buf, int offset);
int fs_add_item(path_string* p_string, FS_ITEM_TYPE type);
bool fs_is_dir(path_string* p_string);
bool fs_is_file(path_string* p_string);
bool fs_item_is_dir(fs_item* item);
bool fs_item_is_file(fs_item* item);
int fs_dir_delete(path_string* p_string);
int fs_file_read(path_string* p_string, char* buffer, size_t size, off_t offset);
int fs_file_write(path_string* p_string, const char* buffer, size_t size, off_t offset);
int fs_file_truncate(path_string* p_string, off_t size);
int fs_file_delete(path_string* p_string);
int fs_rename(path_string* oldpath, path_string* newpath);
int fs_statvfs(path_string* path, struct statvfs* buf);
void init_fs();
void free_fs();

#define fs_foreach(dir_file, key, value) sc_map_foreach(dir_file, key, value)
#define fs_foreach_key(dir_file, key) sc_map_foreach_key(dir_file, key)
#define fs_foreach_val(dir_file, val) sc_map_foreach_value(dir_file, val)

#endif
