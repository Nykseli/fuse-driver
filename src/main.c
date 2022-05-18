
#define FUSE_USE_VERSION 30

#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

static int fdo_mkdir(const char* path, mode_t mode);
static int fdo_getattr(const char* path, struct stat* st);
static int fdo_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
static int fdo_mknod(const char* path, mode_t mode, dev_t rdev);

static struct fuse_operations operations = {
    .getattr = fdo_getattr,
    .readdir = fdo_readdir,
    .mkdir = fdo_mkdir,
    .mknod = fdo_mknod,
};

static int fdo_getattr(const char* path, struct stat* st) {
    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

    if (fs_is_dir(path)) {
        // TODO: set the mode while creating the directory
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
    } else if (fs_is_file(path)) {
        // TODO: set the mode while creating the file
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    } else {
        return -ENOENT;
    }

    return 0;
}

static int fdo_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    filler(buffer, ".", NULL, 0); // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    fs_dir* root;
    int ret = fs_get_directory(path, &root);
    if (ret != 0) {
        return ret;
    }

    const char* key;
    // TODO: filler with stat
    fs_foreach_key(&root->dirs, key) {
        filler(buffer, key, NULL, 0);
    }
    fs_foreach_key(&root->files, key) {
        filler(buffer, key, NULL, 0);
    }
    return 0;
}

static int fdo_mkdir(const char* path, mode_t mode) {
    return fs_add_dir_or_file(path, true);
}

static int fdo_mknod(const char* path, mode_t mode, dev_t rdev) {
    return fs_add_dir_or_file(path, false);
}

int main(int argc, char* argv[]) {
    int ret = 0;
    init_fs();
    ret = fuse_main(argc, argv, &operations, NULL);
    free_fs();
    return ret;
}
