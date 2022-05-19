/* FROM LSYSFS */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

static fs_dir root_dir;

// Get a file from index
char* ps_file(path_string* p_string, int idx) {
    return &p_string->path_copy[p_string->idx_buff[idx]];
}

// Get name of the last file
char* ps_last_file(path_string* p_string) {
    return &p_string->path_copy[p_string->idx_buff[p_string->files - 1]];
}

static void init_fs_file(fs_file* file, const char* name, fs_dir* parent) {
    file->size = 0;
    file->data = NULL;
    file->parent = parent;
    file->name_len = strlen(name);
    file->name = malloc(file->name_len + 1);
    strcpy((char*)file->name, name);
}

static void free_fs_file(fs_file* file) {
    free((void*)file->name);
    if (file->data != NULL) {
        free(file->data);
    }
}

static void init_fs_dir(fs_dir* dir, const char* name, fs_dir* parent) {
    // load_factory arg is the percentage of how full the map can be until realloc
    // default (0) sets the value to 75 so map can be 75% before realloc.
    // this should be fine
    // cap is initial capacity. 0 is accepted so use it
    // TODO: should we prealloc?
    sc_map_init_sv(&dir->dirs, 0, 0);
    sc_map_init_sv(&dir->files, 0, 0);
    dir->parent = parent;
    dir->name_len = strlen(name);
    dir->name = malloc(dir->name_len + 1);
    strcpy((char*)dir->name, name);
}

static void free_fs_dir(fs_dir* dir) {
    fs_file* file;
    fs_foreach_val(&dir->files, file) {
        free_fs_file(file);
    }

    fs_dir* val;
    fs_foreach_val(&dir->dirs, val) {
        free_fs_dir(val);
        sc_map_term_sv(&val->dirs);
        sc_map_term_sv(&val->files);
    }
    free((void*)dir->name);
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

int fs_add_dir_or_file(path_string* p_string, bool is_dir) {
    fs_dir* cdir;
    int ret = fs_get_directory(p_string, &cdir, 1);
    if (ret != 0) {
        return ret;
    }

    char* last_file = ps_last_file(p_string);
    if (is_dir) {
        fs_dir* new_dir = malloc(sizeof(fs_dir));
        init_fs_dir(new_dir, last_file, cdir);
        sc_map_put_sv(&cdir->dirs, new_dir->name, (void*)new_dir);
    } else {
        fs_file* new_file = malloc(sizeof(fs_file));
        init_fs_file(new_file, last_file, cdir);
        sc_map_put_sv(&cdir->files, new_file->name, (void*)new_file);
    }

    return 0;
}

int fs_dir_delete(path_string* p_string) {
    fs_dir* dir;
    int ret = fs_get_directory(p_string, &dir, 0);
    if (ret != 0) {
        return ret;
    }

    if (dir->files.size != 0 || dir->dirs.size) {
        return -ENOTEMPTY;
    }

    // TODO: can we just assume that this always works?
    sc_map_del_sv(&dir->parent->dirs, dir->name);
    free_fs_dir(dir);
    return 0;
}

/**
 * Get directory. If returns != 0, error occurred.
 * Offset decides the dir relative to path. 0 is last, 1 one before that etc.
 * If offset is < 0, it's set to 0
 */
int fs_get_directory(path_string* p_string, fs_dir** buf, int offset) {
    if (offset < 0) {
        offset = 0;
    }

    fs_dir* cdir = &root_dir;
    for (int ii = 0; ii < p_string->files - offset; ii++) {
        char* p = ps_file(p_string, ii);
        fs_dir* next = sc_map_get_sv(&cdir->dirs, p);
        if (!sc_map_found(&cdir->dirs)) {
            return -ENOENT;
        }

        cdir = next;
    }

    if (buf != NULL)
        *buf = cdir;
    return 0;
}

/**
 * Get file. If returns != 0, error occurred.
 */
int fs_get_file(path_string* p_string, fs_file** buf) {
    fs_dir* cdir;
    int ret = fs_get_directory(p_string, &cdir, 1);
    if (ret != 0) {
        return ret;
    }

    fs_file* file = sc_map_get_sv(&cdir->files, ps_last_file(p_string));
    if (!sc_map_found(&cdir->files)) {
        return -ENOENT;
    }

    if (buf != NULL)
        *buf = file;
    return 0;
}

// TODO: create function that checks if path is file, dir or non existing
//       so we don't need to double check
bool fs_is_dir(path_string* p_string) {
    return fs_get_directory(p_string, NULL, 0) == 0;
}

void init_fs() {
    init_fs_dir(&root_dir, "/", NULL);
}

void free_fs() {
    free_fs_dir(&root_dir);
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
    fs_file* file;
    int ret = fs_get_file(p_string, &file);
    if (ret != 0) {
        return ret;
    }

    // TODO: can we just assume that this always works?
    sc_map_del_sv(&file->parent->files, file->name);
    free_fs_file(file);
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

    int ret = 0;

    fs_dir* old_parent = NULL;
    ret = fs_get_directory(oldpath, &old_parent, 1);
    if (ret != 0) {
        return ret;
    }

    // Are we moving a file or a dir
    bool is_old_dir = false;
    bool old_found = false;

    fs_dir* old_dir = sc_map_get_sv(&old_parent->dirs, ps_last_file(oldpath));
    if (sc_map_found(&old_parent->dirs)) {
        is_old_dir = true;
        old_found = true;
    }

    fs_file* old_file = NULL;
    if (!old_found) {
        old_file = sc_map_get_sv(&old_parent->files, ps_last_file(oldpath));
        if (sc_map_found(&old_parent->files)) {
            old_found = true;
        }
    }

    if (!old_found) {
        return -ENOENT;
    }

    // After we actually know that the file exist, we need to figure out if
    // we can move it to the new path

    fs_dir* new_parent = NULL;
    ret = fs_get_directory(newpath, &new_parent, 1);
    if (ret != 0) {
        return ret;
    }

    bool is_new_dir = false;
    bool new_found = false;

    fs_dir* new_dir = sc_map_get_sv(&new_parent->dirs, ps_last_file(newpath));
    if (sc_map_found(&new_parent->dirs)) {
        is_new_dir = true;
        new_found = true;
    }

    fs_file* new_file = NULL;
    if (!new_found) {
        new_file = sc_map_get_sv(&new_parent->files, ps_last_file(newpath));
        if (sc_map_found(&new_parent->files)) {
            new_found = true;
        }
    }

    // if the new path doesn't exist, we can just move the old path to it
    if (!new_found) {
        if (is_old_dir) {
            sc_map_del_sv(&old_parent->dirs, old_dir->name);
            free((void*)old_dir->name);
            size_t nlen = strlen(ps_last_file(newpath));
            old_dir->name = malloc(nlen + 1);
            old_dir->name_len = nlen;
            strcpy((char*)old_dir->name, ps_last_file(newpath));
            sc_map_put_sv(&new_parent->dirs, old_dir->name, old_dir);
        } else {
            sc_map_del_sv(&old_parent->files, old_file->name);
            free((void*)old_file->name);
            size_t nlen = strlen(ps_last_file(newpath));
            old_file->name = malloc(nlen + 1);
            old_file->name_len = nlen;
            strcpy((char*)old_file->name, ps_last_file(newpath));
            sc_map_put_sv(&new_parent->files, old_file->name, old_file);
        }
    } else { // new is found
        if (is_old_dir) {
            if (is_new_dir) {
                // We cannot override non-empty dirs
                if (new_dir->dirs.size != 0 || new_dir->dirs.size != 0)
                    return -ENOTEMPTY;

                sc_map_del_sv(&old_parent->dirs, old_dir->name);
                sc_map_put_sv(&new_dir->dirs, new_dir->name, old_dir);
                free_fs_dir(new_dir);
            } else {
                // cannot overwrite non-directory with directory
                return -EPERM;
            }
        } else { // old is file
            sc_map_del_sv(&old_parent->files, old_file->name);
            if (is_new_dir) { // move the old file to new directory
                sc_map_put_sv(&new_dir->files, old_file->name, old_file);
            } else { // new is also a file so we override it
                sc_map_put_sv(&new_parent->files, old_file->name, old_file);
                free_fs_file(new_file);
            }
        }
    }

    return 0;
}
