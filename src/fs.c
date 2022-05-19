#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

// root is directory type item
static fs_item root_dir;

static void free_fs_item(fs_item* item);
static int fs_get_dir_item(path_string* p_string, fs_item** buf, int offset);

// Get a file from index
char* ps_file(path_string* p_string, int idx) {
    return &p_string->path_copy[p_string->idx_buff[idx]];
}

// Get name of the last file
char* ps_last_file(path_string* p_string) {
    return &p_string->path_copy[p_string->idx_buff[p_string->files - 1]];
}

bool fs_item_is_dir(fs_item* item) {
    return item->type == FS_DIR;
}

bool fs_item_is_file(fs_item* item) {
    return item->type == FS_FILE;
}

static void init_fs_file(fs_file* file) {
    file->size = 0;
    file->data = NULL;
}

static void free_fs_file(fs_file* file) {
    if (file->data != NULL) {
        free(file->data);
    }
}

static void init_fs_dir(fs_dir* dir) {
    // load_factory arg is the percentage of how full the map can be until realloc
    // default (0) sets the value to 75 so map can be 75% before realloc.
    // this should be fine
    // cap is initial capacity. 0 is accepted so use it
    // TODO: should we prealloc?
    sc_map_init_sv(&dir->items, 0, 0);
}

static void free_fs_dir(fs_dir* dir) {
    fs_item* item;
    fs_foreach_val(&dir->items, item) {
        free_fs_item(item);
        free(item);
    }
    sc_map_term_sv(&dir->items);
}

static void init_fs_item(fs_item* item, const char* name, fs_item* parent, FS_ITEM_TYPE type) {
    item->type = type;
    item->parent = parent;
    item->name_len = strlen(name);
    item->name = malloc(item->name_len + 1);
    strcpy((char*)item->name, name);
    if (fs_item_is_dir(item)) {
        init_fs_dir(&fs_item_dir(item));
    } else {
        init_fs_file(&fs_item_file(item));
    }
}

static void free_fs_item(fs_item* item) {
    if (fs_item_is_dir(item)) {
        free_fs_dir(&fs_item_dir(item));
    } else {
        free_fs_file(&fs_item_file(item));
    }
    free((char*)item->name);
}

/**
 * Split file path into indexes and set the indexes to the buffer.
 * path string is modified as a side effect
 * return the amount of indexes found.
 * Will return 0 on root dir "/", >= on any other path
 *
 * Usage:
 *
 * char path_copy[PATH_LEN_MAX];
 * strcpy(path_copy, path);
 * int idx_buff[256];
 * int files = split_file_path(path_copy, idx_buff);
 * for (int ii = 0; ii < files; ii++) {
 *     printf("splitted: %s\n", &path_copy[idx_buff[ii]]);
 * }
 */
static int split_file_path(char* path, int* idx_buff) {
    int idx_count = 1;
    size_t idx = 0;
    size_t path_len = strlen(path);

    // If checking root
    if (path_len == 1 && path[0] == '/') {
        return 0;
    }

    if (path[0] == '/') {
        idx = 1;
        idx_buff[0] = 1;
    } else {
        idx_buff[0] = 0;
    }

    for (; idx < path_len; idx++) {
        if (path[idx] == '/') {
            // Set the splitting point
            path[idx] = '\0';
            // The start of next path is the next char after null
            idx_buff[idx_count++] = idx + 1;
        }
    }

    return idx_count;
}

int create_path_string(path_string* p_string, const char* path) {
    p_string->path = path;
    strcpy(p_string->path_copy, path);
    p_string->files = split_file_path(p_string->path_copy, p_string->idx_buff);

    return 0;
}

bool fs_is_file(path_string* p_string) {
    // root dir is not a file
    if (p_string->files == 0) {
        return false;
    }

    return fs_get_file(p_string, NULL) == 0;
}

int fs_add_item(path_string* p_string, FS_ITEM_TYPE type) {
    fs_item* cdir;
    int ret = fs_get_dir_item(p_string, &cdir, 1);
    if (ret != 0)
        return ret;

    char* last_file = ps_last_file(p_string);
    fs_item* new_item = malloc(sizeof(fs_item));
    init_fs_item(new_item, last_file, cdir, type);
    sc_map_put_sv(&fs_item_dir(cdir).items, new_item->name, (void*)new_item);
    return 0;
}

int fs_dir_delete(path_string* p_string) {
    fs_item* item;
    int ret = fs_get_dir_item(p_string, &item, 0);
    if (ret != 0) {
        return ret;
    }

    if (fs_item_dir(item).items.size != 0) {
        return -ENOTEMPTY;
    }

    // TODO: can we just assume that this always works?
    sc_map_del_sv(&fs_item_dir(item->parent).items, item->name);
    free_fs_item(item);
    free(item);
    return 0;
}

int fs_get_item(path_string* p_string, fs_item** buf, int offset) {
    if (offset < 0) {
        offset = 0;
    }

    fs_dir* cdir = &fs_item_dir(&root_dir);
    fs_item* found = &root_dir;
    int loop_count = p_string->files - offset;
    for (int ii = 0; ii < loop_count; ii++) {
        char* p = ps_file(p_string, ii);
        fs_item* next = sc_map_get_sv(&cdir->items, p);
        if (!sc_map_found(&cdir->items)) {
            return -ENOENT;
        } else if (ii < loop_count - 1 && !fs_item_is_dir(next)) {
            // Files one before the lastone always need to be directories
            return -ENOTDIR;
        }

        cdir = &fs_item_dir(next);
        found = next;
    }

    if (buf != NULL)
        *buf = found;

    return 0;
}

/**
 * Get fs_item that is always a directory type.
 */
int fs_get_dir_item(path_string* p_string, fs_item** buf, int offset) {
    fs_item* found;
    int ret = fs_get_item(p_string, &found, offset);
    if (ret != 0) {
        return ret;
    } else if (!fs_item_is_dir(found)) {
        return -ENOTDIR;
    }

    if (buf != NULL)
        *buf = found;

    return 0;
}

/**
 * Get directory. If returns != 0, error occurred.
 * Offset decides the dir relative to path. 0 is last, 1 one before that etc.
 * If offset is < 0, it's set to 0
 */
int fs_get_directory(path_string* p_string, fs_dir** buf, int offset) {
    fs_item* dir;
    int ret = fs_get_dir_item(p_string, &dir, offset);
    if (ret != 0)
        return ret;

    if (buf != NULL)
        *buf = &fs_item_dir(dir);

    return 0;
}

/**
 * Get fs_item that is always a file type.
 */
int fs_get_file_item(path_string* p_string, fs_item** buf) {
    fs_item* found;
    int ret = fs_get_item(p_string, &found, 0);
    if (ret != 0) {
        return ret;
    } else if (fs_item_is_dir(found)) {
        return -EISDIR;
    }

    if (buf != NULL)
        *buf = found;

    return 0;
}

/**
 * Get file. If returns != 0, error occurred.
 */
int fs_get_file(path_string* p_string, fs_file** buf) {
    fs_item* dir;
    int ret = fs_get_file_item(p_string, &dir);
    if (ret != 0)
        return ret;

    if (buf != NULL)
        *buf = &fs_item_file(dir);

    return 0;
}

// TODO: create function that checks if path is file, dir or non existing
//       so we don't need to double check
bool fs_is_dir(path_string* p_string) {
    return fs_get_directory(p_string, NULL, 0) == 0;
}

void init_fs() {
    init_fs_item(&root_dir, "/", NULL, FS_DIR);
}

void free_fs() {
    free_fs_item(&root_dir);
}

int fs_file_read(path_string* p_string, char* buffer, size_t size, off_t offset) {
    fs_file* file;
    int ret = fs_get_file(p_string, &file);
    if (ret != 0) {
        return ret;
    }

    if (file->data == NULL && file->size == 0) {
        return 0;
    } else if (size > file->size - offset) {
        size = file->size - offset;
    }

    memcpy(buffer, file->data + offset, size);
    return size;
}

int fs_file_write(path_string* p_string, const char* buffer, size_t size, off_t offset) {
    fs_file* file;
    int ret = fs_get_file(p_string, &file);
    if (ret != 0) {
        return ret;
    }

    // if offset is not part of the file, the file will end up containing garbage
    // TODO: what does offset < 0 officially mean?
    if (offset < 0 || file->size < (size_t)offset) {
        return -ESPIPE;
    }

    file->size = file->size + size;
    if (file->data != NULL) {
        file->data = realloc(file->data, file->size);
    } else {
        file->data = malloc(file->size);
    }

    memcpy(file->data + offset, buffer, size);
    return size;
}

int fs_file_truncate(path_string* p_string, off_t size) {
    fs_file* file;
    int ret = fs_get_file(p_string, &file);
    if (ret != 0) {
        return ret;
    }

    // If size it is negative, remove the size value from file size
    if (size < 0) {
        // We cannot trunk the file size to be < 0
        if (file->size < (size_t)(size * -1))
            return -ESPIPE;

        file->size = file->size + size;
    } else if (file->size < (size_t)size) {
        return -ESPIPE;
    } else {
        // else we set the size to size
        file->size = size;
    }

    return 0;
}

int fs_file_delete(path_string* p_string) {
    fs_item* file;
    int ret = fs_get_file_item(p_string, &file);
    if (ret != 0) {
        return ret;
    }

    // TODO: can we just assume that this always works?
    sc_map_del_sv(&fs_item_dir(file->parent).items, file->name);
    free_fs_item(file);
    free(file);
    return 0;
}

/**
 * rename logic from rename man page (man 2 rename)
 *
 * If newpath already exists, it will be atomically replaced, so that there is
 * no point at which another process attempting to access newpath will find it missing.
 * However, there will probably be a window in which both oldpath and newpath
 * refer to the file being renamed.
 *
 * TODO: implement this when we have hard links
 * If oldpath and newpath are existing hard links referring to the same file, then rename() does nothing, and returns a success status.
 *
 * If newpath exists but the operation fails for some reason, rename() guarantees to leave an instance of newpath in place.
 *
 * oldpath can specify a directory. In this case, newpath must either not exist, or it must specify an empty directory.
 *
 * TODO: implement this when we have symbolic links
 * If oldpath refers to a symbolic link, the link is renamed; if newpath refers to a symbolic link, the link will be overwritten.
 */
int fs_rename(path_string* oldpath, path_string* newpath) {
    // we cannot move the root file
    if (oldpath->files == 0) {
        return -EPERM;
    }

    fs_item* old_item;
    int ret = fs_get_item(oldpath, &old_item, 0);
    if (ret != 0) {
        return ret;
    }

    fs_dir* old_parent = &fs_item_dir(old_item->parent);
    bool is_old_dir = fs_item_is_dir(old_item);

    fs_item* new_parent_item = NULL;
    ret = fs_get_dir_item(newpath, &new_parent_item, 1);
    if (ret != 0)
        return ret;

    fs_dir* new_parent = &fs_item_dir(new_parent_item);

    bool new_found = false;
    fs_item* new_item = sc_map_get_sv(&new_parent->items, ps_last_file(newpath));
    if (sc_map_found(&new_parent->items)) {
        new_found = true;
    }

    fs_dir* new_dir = &fs_item_dir(new_item);
    fs_file* new_file = &fs_item_file(new_item);
    bool is_new_dir = new_found && fs_item_is_dir(new_item);

    // if the new path doesn't exist, we can just move the old path to it
    if (!new_found) {
        sc_map_del_sv(&old_parent->items, old_item->name);
        free((void*)old_item->name);
        size_t nlen = strlen(ps_last_file(newpath));
        old_item->name = malloc(nlen + 1);
        old_item->name_len = nlen;
        old_item->parent = new_parent_item;
        strcpy((char*)old_item->name, ps_last_file(newpath));
        sc_map_put_sv(&new_parent->items, old_item->name, old_item);
    } else { // new is found
        if (is_old_dir) {
            // cannot overwrite non-directory with directory
            if (!is_new_dir)
                return -EPERM;
            // We cannot override non-empty dirs
            if (new_dir->items.size != 0)
                return -ENOTEMPTY;

            old_item->parent = new_parent_item;
            sc_map_del_sv(&old_parent->items, old_item->name);
            sc_map_put_sv(&new_dir->items, new_item->name, old_item);
            free_fs_dir(new_dir);
            free(new_dir);
        } else { // old is file
            old_item->parent = new_parent_item;
            sc_map_del_sv(&old_parent->items, old_item->name);
            if (is_new_dir) { // move the old file to new directory
                sc_map_put_sv(&new_dir->items, old_item->name, old_item);
            } else { // new is also a file so we override it
                sc_map_put_sv(&new_parent->items, old_item->name, old_item);
                free_fs_file(new_file);
                free(new_file);
            }
        }
    }

    return 0;
}
