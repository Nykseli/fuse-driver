/* FROM LSYSFS */

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

// TODO: splitting path: memcopy the dirname into buffer, inhe buffer found the / collect the indexes and set them to null
//       then it's easy to start the next level from the next index

int fs_add_dir(const char* dir_name) {
    dir_name++; // remove the first /
    fs_dir* new_dir = malloc(sizeof(fs_dir));
    init_fs_dir(new_dir, dir_name);
    sc_map_put_sv(&root_dir.dirs, new_dir->name, (void*)new_dir);
    return 0;
}

bool fs_is_dir(const char* path) {
    path++; // Eliminating "/" in the path

    sc_map_get_sv(&root_dir.dirs, path);
    return sc_map_found(&root_dir.dirs);
}

void init_fs() {
    init_fs_dir(&root_dir, "/");
}

void free_fs() {
    free_fs_dir(&root_dir);
}
