
#include <errno.h>
#include <fuse.h>
#include <stdio.h>

#include "fs_fh.h"
#include "sc_map.h"

typedef struct fs_fh_pid {
    pid_t pid;
    // what what next opened file should have as a fd
    int next_fd;
    // int 32 key -> fs_item* value
    // TODO: could we replace this with a 256 lenght list since it's the max amount
    //       of open files we are going to support
    //       see fs/file.c SYSCALL_DEFINE3(open, from kernel source to see how kernel assigns the fd
    struct sc_map_32v items;
} fs_fh_pid;

struct sc_map_32v fs_pids;

#define split_handle(_fh, _pid, _fd) \
    int _fd = (int)_fh;              \
    pid_t _pid = (pid_t)(_fh >> 48)

static fs_fh_pid* new_ffp(pid_t pid) {
    fs_fh_pid* new = malloc(sizeof(fs_fh_pid));
    new->pid = pid;
    new->next_fd = 1;
    sc_map_init_32v(&new->items, 0, 0);
    return new;
}

static file_handle add_file(pid_t pid, fs_item* item) {
    fs_fh_pid* ffp = sc_map_get_32v(&fs_pids, pid);
    if (!sc_map_found(&fs_pids)) {
        ffp = new_ffp(pid);
        sc_map_put_32v(&fs_pids, pid, ffp);
    }

    sc_map_put_32v(&ffp->items, ffp->next_fd, item);
    file_handle handle = ((uint64_t)pid << 48) + ffp->next_fd;
    ffp->next_fd += 1;
    return handle;
}

file_handle fs_fh_file_handle(fs_item* item) {
    // TODO: too many files open error
    pid_t pid = fuse_get_context()->pid;
    return add_file(pid, item);
}

void fs_fh_release_file(file_handle fh) {
    split_handle(fh, pid, fd);
    fs_fh_pid* ffp = sc_map_get_32v(&fs_pids, pid);
    if (sc_map_found(&fs_pids)) {
        sc_map_del_32v(&ffp->items, fd);
    }
}

int fs_fh_get_item(file_handle fh, fs_item** buf) {
    split_handle(fh, pid, fd);
    fs_fh_pid* ffp = sc_map_get_32v(&fs_pids, pid);
    if (!sc_map_found(&fs_pids))
        return -1; // process is terminated

    fs_item* item = sc_map_get_32v(&ffp->items, fd);
    if (!sc_map_found(&ffp->items))
        return -1; // file is closed

    *buf = item;
    return 0;
}

int fs_fh_get_file(file_handle fh, fs_file** buf) {
    fs_item* found;
    int ret = fs_fh_get_item(fh, &found);
    if (ret != 0) {
        return ret;
    } else if (!fs_item_is_file(found)) {
        return -EISDIR;
    }

    *buf = &fs_item_file(found);
    return 0;
}

int fs_fh_get_dir(file_handle fh, fs_dir** buf) {
    fs_item* found;
    int ret = fs_fh_get_item(fh, &found);
    if (ret != 0) {
        return ret;
    } else if (!fs_item_is_dir(found)) {
        return -ENOTDIR;
    }

    *buf = &fs_item_dir(found);
    return 0;
}

void init_fs_fh() {
    sc_map_init_32v(&fs_pids, 0, 0);
    // TODO: create separate gc thread that cleans the maps of processes that are nolonger running
}
void free_fs_fh() {
    fs_fh_pid* pid;
    sc_map_foreach_value(&fs_pids, pid) {
        sc_map_term_32v(&pid->items);
        free(pid);
    }
    sc_map_term_32v(&fs_pids);
    // TODO: stop gc
}
