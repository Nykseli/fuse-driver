#ifndef FS_H
#define FS_H

#include <fuse.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#include "sc_map.h"
#include "util.h"

// 4096 is a good cache friendly size
#define FS_BLOCK_SIZE 4096

typedef enum FS_ITEM_TYPE {
    FS_DIR,
    FS_FILE
} FS_ITEM_TYPE;

struct fs_item;

typedef struct fs_dir {
    struct fs_item* item;
    struct sc_map_sv items;
} fs_dir;

typedef struct fs_file {
    struct fs_item* item;
    // Data length can be found from item's stat struct (st_size)
    uint8_t* data;
} fs_file;

#if FILE_NAME_MAX > 255
#error "fs_item name_len only supports values that fit into uint8_t "
#endif

typedef struct fs_item {
    const char name[FILE_NAME_MAX + 1];
    uint8_t name_len;
    // null if root dir
    // will always point to a fs_dir item
    struct fs_item* parent;
    // TODO: stat and replace type with it
    struct stat st;
    union {
        fs_dir dir;
        fs_file file;
    } as;
} fs_item;

typedef struct path_string {
    // pointer to the original from fuse funtions
    const char* path;

    // copy of the path containting the divided paths
    char path_copy[PATH_LEN_MAX + 1];
    // indexes of the paths
    // TODO: collect the length of paths so we do faster map lookups
    //       this requires some changes in sc_map
    int idx_buff[256];
    // count of the indexes, if 0. this is the root file
    int files;
} path_string;

#define fs_item_dir(_item) (_item)->as.dir
#define fs_item_file(_item) (_item)->as.file
// size of the union items
#define fs_item_size(_item) (_item)->item->st.st_size

int parse_path_string(path_string* p_string, const char* path);
/**
 * Generate path_string and make sure that the path or name is not too long
 */
#define create_path_string(_pstring, _path)                  \
    do {                                                     \
        int ret;                                             \
        if ((ret = parse_path_string(_pstring, _path)) != 0) \
            return ret;                                      \
    } while (0)

// TODO: make fs_ to reflect syscalls. fs_read, fs_unlink etc

int fs_get_file(path_string* p_string, fs_file** buf);
int fs_get_item(path_string* p_string, fs_item** buf, int offset);
int fs_get_directory(path_string* p_string, fs_dir** buf, int offset);
int fs_dir_delete(path_string* p_string);
int fs_file_read(path_string* p_string, char* buffer, size_t size, off_t offset);
int fs_file_write(path_string* p_string, const char* buffer, size_t size, off_t offset);
int fs_file_truncate(path_string* p_string, off_t size);
int fs_file_delete(path_string* p_string);
int fs_rename(path_string* oldpath, path_string* newpath);
int fs_statvfs(path_string* path, struct statvfs* buf);
int fs_mknod(path_string* path, mode_t mode, dev_t rdev);
int fs_mkdir(path_string* path, mode_t mode);
int fs_chown(path_string* path, uid_t uid, gid_t gid);
int fs_chmod(path_string* path, mode_t mode);
int fs_access(path_string* path, mode_t mode);
void init_fs();
void free_fs();

#define fs_foreach(dir_file, key, value) sc_map_foreach(dir_file, key, value)
#define fs_foreach_key(dir_file, key) sc_map_foreach_key(dir_file, key)
#define fs_foreach_val(dir_file, val) sc_map_foreach_value(dir_file, val)

#endif
