/* FROM LSYSFS */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

static fs_dir root_dir;

// Get a file from index
#define ps_file(pstring, idx) &(pstring)->path_copy[(pstring)->idx_buff[ii]];
// Get name of the last file
#define ps_last_file(pstring) &(pstring)->path_copy[(pstring)->idx_buff[(pstring)->files - 1]];

static void init_fs_file(fs_file* file, const char* name) {
    file->size = 0;
    file->data = NULL;
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

    // find the directory that's one before the target file
    fs_dir* cdir = &root_dir;
    for (int ii = 0; ii < p_string->files - 1; ii++) {
        char* p = ps_file(p_string, ii);
        fs_dir* next = sc_map_get_sv(&cdir->dirs, p);
        if (!sc_map_found(&cdir->dirs)) {
            return false;
        }

        cdir = next;
    }

    char* last_file = ps_last_file(p_string);
    sc_map_get_sv(&cdir->files, last_file);
    return sc_map_found(&cdir->files);
}

int fs_add_dir_or_file(path_string* p_string, bool is_dir) {
    fs_dir* cdir = &root_dir;

    // loop until the last dir before the last is found
    for (int ii = 0; ii < p_string->files - 1; ii++) {
        char* p = ps_file(p_string, ii);
        fs_dir* next = sc_map_get_sv(&cdir->dirs, p);
        if (!sc_map_found(&cdir->dirs)) {
            return -ENOENT;
        }
        cdir = next;
    }

    char* last_file = ps_last_file(p_string);
    if (is_dir) {
        fs_dir* new_dir = malloc(sizeof(fs_dir));
        init_fs_dir(new_dir, last_file);
        sc_map_put_sv(&cdir->dirs, new_dir->name, (void*)new_dir);
    } else {
        fs_file* new_file = malloc(sizeof(fs_file));
        init_fs_file(new_file, last_file);
        sc_map_put_sv(&cdir->files, new_file->name, (void*)new_file);
    }

    return 0;
}

/**
 * Get directory. If returns != 0, error occurred
 */
int fs_get_directory(path_string* p_string, fs_dir** buf) {
    fs_dir* cdir = &root_dir;
    for (int ii = 0; ii < p_string->files; ii++) {
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

// TODO: create function that checks if path is file, dir or non existing
//       so we don't need to double check
bool fs_is_dir(path_string* p_string) {
    return fs_get_directory(p_string, NULL) == 0;
}

void init_fs() {
    init_fs_dir(&root_dir, "/");
}

void free_fs() {
    free_fs_dir(&root_dir);
}
