
#include "util.h"

#include <errno.h>
#include <fuse.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "fs_fh.h"
#include "sc_map.h"

// pid is 32 bits so 64-32 = 32
#define PID_OFFSET 32

static file_handle add_file(pid_t pid, const fs_item* item) __nonnull((2));

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
pthread_t clean_thread;

#define split_handle(_fh, _pid, _fd) \
    int _fd = (int)_fh;              \
    pid_t _pid = (pid_t)(_fh >> PID_OFFSET)

static void* pid_clean_fn(void* unused) {
    pid_t pid;
    fs_fh_pid* pobj;
    while (true) {
        sc_map_foreach(&fs_pids, pid, pobj) {
            // If sig is 0, then no signal is sent, but existence and
            // permission checks are still performed; this can be used to check
            // for the existence of a process ID or process group ID that the
            // caller is per-mitted to signal.
            // TODO: what happens if we don't have a persmission to send a signal?
            if (kill(pid, 0) == -1) {
                sc_map_term_32v(&pobj->items);
                free(pobj);
                sc_map_del_32v(&fs_pids, pid);
            }
        }

        sleep_ms(1000); // is 1 second too agressive?
    }

    return NULL;
}

static fs_fh_pid* new_ffp(pid_t pid) {
    fs_fh_pid* new = malloc(sizeof(fs_fh_pid));
    new->pid = pid;
    new->next_fd = 1;
    sc_map_init_32v(&new->items, 0, 0);
    return new;
}

static file_handle add_file(pid_t pid, const fs_item* item) {
    fs_fh_pid* ffp = sc_map_get_32v(&fs_pids, pid);
    if (!sc_map_found(&fs_pids)) {
        ffp = new_ffp(pid);
        sc_map_put_32v(&fs_pids, pid, ffp);
    }

    sc_map_put_32v(&ffp->items, ffp->next_fd, (void*)item);
    file_handle handle = ((uint64_t)pid << PID_OFFSET) + ffp->next_fd;
    ffp->next_fd += 1;
    return handle;
}

file_handle fs_fh_file_handle(const fs_item* item) {
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
        return -EBADF; // process is terminated

    fs_item* item = sc_map_get_32v(&ffp->items, fd);
    if (!sc_map_found(&ffp->items))
        return -EBADF; // file is closed

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
    pthread_create(&clean_thread, NULL, pid_clean_fn, NULL);
}

void free_fs_fh() {
    fs_fh_pid* pid;
    sc_map_foreach_value(&fs_pids, pid) {
        sc_map_term_32v(&pid->items);
        free(pid);
    }
    sc_map_term_32v(&fs_pids);
    pthread_cancel(clean_thread);
}
