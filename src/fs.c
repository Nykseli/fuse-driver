/* FROM LSYSFS */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

fs_dir root_dir;

fs_dir* get_root_dir() {
    return &root_dir;
}

static void init_fs_dir(fs_dir* dir, const char* name) {
    // load_factory arg is the percentage of how full the map can be until realloc
    // default (0) sets the value to 75 so map can be 75% before realloc.
    // this should be fine
    // cap is initial capacity. 0 is accepted so use it
    // TODO: should we prealloc?
    sc_map_init_sv(&dir->dirs, 0, 0);
    sc_map_init_sv(&dir->files, 0, 0);
    dir->name_len = strlen(name);
    dir->name = malloc(dir->name_len + 1);
    strcpy((char*)dir->name, name);
}

static void free_fs_dir(fs_dir* dir) {
    fs_dir* val;
    fs_foreach_val(&dir->dirs, val) {
        free_fs_dir(val);
        sc_map_term_sv(&val->dirs);
        sc_map_term_sv(&val->files);
    }

    // TODO: go through files and free them too

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

int fs_add_dir(const char* dir_path) {

    char path_copy[PATH_LEN_MAX];
    strcpy(path_copy, dir_path);

    int idx_buff[256];
    int files = split_file_path(path_copy, idx_buff);
    fs_dir* cdir = &root_dir;

    // loop until the last dir before the last is found
    for (int ii = 0; ii < files - 1; ii++) {
        char* p = &path_copy[idx_buff[ii]];
        fs_dir* next = sc_map_get_sv(&cdir->dirs, p);
        if (!sc_map_found(&cdir->dirs)) {
            return -ENOENT;
        }
        cdir = next;
    }

    fs_dir* new_dir = malloc(sizeof(fs_dir));
    char* last_file = &path_copy[idx_buff[files - 1]];
    init_fs_dir(new_dir, last_file);
    sc_map_put_sv(&cdir->dirs, new_dir->name, (void*)new_dir);
    return 0;
}

/**
 * Get directory. If returns != 0, error occurred
 */
int fs_get_directory(const char* path, fs_dir** buf) {
    char path_copy[PATH_LEN_MAX];
    strcpy(path_copy, path);

    int idx_buff[256];
    int files = split_file_path(path_copy, idx_buff);
    fs_dir* cdir = &root_dir;

    for (int ii = 0; ii < files; ii++) {
        char* p = &path_copy[idx_buff[ii]];
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

// TODO: create function that checks if path is file, dir or non existing
//       so we don't need to double check
bool fs_is_dir(const char* path) {
    return fs_get_directory(path, NULL) == 0;
}

void init_fs() {
    init_fs_dir(&root_dir, "/");
}

void free_fs() {
    free_fs_dir(&root_dir);
}
