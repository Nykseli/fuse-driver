
#define FUSE_USE_VERSION 30

#include "util.h"

#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "fs.h"
#include "fs_fh.h"

static int fdo_mkdir(const char* path, mode_t mode);
static int fdo_getattr(const char* path, struct stat* st, struct fuse_file_info* fi);
static int fdo_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);
static int fdo_mknod(const char* path, mode_t mode, dev_t rdev);
static int fdo_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi);
static int fdo_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi);
static int fdo_truncate(const char* path, off_t size, struct fuse_file_info* fi);
static int fdo_unlink(const char* path);
static int fdo_rmdir(const char* path);
static int fdo_rename(const char* oldpath, const char* newpath, unsigned int flags);
static int fdo_symlink(const char* linkname, const char* path);
static int fdo_link(const char* oldpath, const char* newpath);
static int fdo_open(const char* path, struct fuse_file_info* fi);
static int fdo_release(const char* path, struct fuse_file_info* fi);
static int fdo_fsync(const char* path, int datasync, struct fuse_file_info* fi);
static int fdo_flush(const char* path, struct fuse_file_info* fi);
static int fdo_statfs(const char* path, struct statvfs* buf);
static int fdo_opendir(const char* path, struct fuse_file_info* fi);
static int fdo_releasedir(const char* path, struct fuse_file_info* fi);
static int fdo_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
static int fdo_chmod(const char* path, mode_t mode, struct fuse_file_info* fi);
static int fdo_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi);
static int fdo_utimens(const char* path, const struct timespec tv[2], struct fuse_file_info* fi);
static int fdo_access(const char* path, int mask);
static int fdo_readlink(const char* path, char* buf, size_t len);
static int fdo_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi, unsigned int flags, void* data);
static int fdo_poll(const char* path, struct fuse_file_info* fi, struct fuse_pollhandle* ph, unsigned* reventsp);
static int fdo_fallocate(const char* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi);

static struct fuse_operations operations = {
    .getattr = fdo_getattr,
    .readlink = fdo_readlink,
    .mknod = fdo_mknod,
    .mkdir = fdo_mkdir,
    .unlink = fdo_unlink,
    .rmdir = fdo_rmdir,
    .symlink = fdo_symlink,
    .rename = fdo_rename,
    .link = fdo_link,
    .chmod = fdo_chmod,
    .chown = fdo_chown,
    .truncate = fdo_truncate,
    .open = fdo_open,
    .read = fdo_read,
    .write = fdo_write,
    .statfs = fdo_statfs,
    .flush = fdo_flush,
    .fsync = fdo_fsync,
    .opendir = fdo_opendir,
    .releasedir = fdo_releasedir,
    .readdir = fdo_readdir,
    .fsyncdir = fdo_fsyncdir,
    .access = fdo_access,
    .utimens = fdo_utimens,
    .poll = fdo_poll,
    .fallocate = fdo_fallocate,
    .ioctl = fdo_ioctl,
    .release = fdo_release,
    // we don't have any xattrs
    // .setxattr = fdo_setxattr,
    // .getxattr = fdo_getxattr,
    // .listxattr = fdo_listxattr,
    // .removexattr = fdo_removexattr,
    // mknod can handdle the create calls
    // .create = fdo_create,
    // let kernel handle the locks
    // .lock = fdo_lock,
    // .flock = fdo_flock,
    // let normal writes and reads handle read/write
    // .write_buf = fdo_write_buf,
    // .read_buf = fdo_read_buf,
};

static int fdo_getattr(const char* path, struct stat* st, struct fuse_file_info* fi) {
    int ret;
    fs_item* item;
    path_string p_string;

    if (fi != NULL) {
        ret = fs_fh_get_item(fi->fh, &item);
    } else {
        create_path_string(&p_string, path);
        ret = fs_get_item(&p_string, &item, 0);
    }

    if (ret != 0)
        return ret;

    memcpy(st, &item->st, sizeof(struct stat));
    return 0;
}

static int fdo_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    fs_dir* root;
    int ret = fs_fh_get_dir(fi->fh, &root);
    if (ret != 0) {
        return ret;
    }

    filler(buffer, ".", NULL, 0, 0); // Current Directory
    filler(buffer, "..", NULL, 0, 0); // Parent Directory

    const char* key;
    const fs_item* item;
    fs_foreach(&root->items, key, item) {
        filler(buffer, key, &item->st, 0, 0);
    }
    return 0;
}

static int fdo_mkdir(const char* path, mode_t mode) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_mkdir(&p_string, mode);
}

static int fdo_mknod(const char* path, mode_t mode, dev_t rdev) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_mknod(&p_string, mode, rdev);
}

static int fdo_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
    return fs_read(fi->fh, buffer, size, offset);
}

static int fdo_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
    return fs_write(fi->fh, buffer, size, offset);
}

static int fdo_truncate(const char* path, off_t size, struct fuse_file_info* fi) {
    path_string p_string;
    if (fi != NULL) {
        return fs_ftruncate(fi->fh, size);
    } else {
        create_path_string(&p_string, path);
        return fs_truncate(&p_string, size);
    }
}

static int fdo_unlink(const char* path) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_file_delete(&p_string);
}

static int fdo_rmdir(const char* path) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_dir_delete(&p_string);
}

static int fdo_rename(const char* oldpath, const char* newpath, unsigned int flags) {
    path_string old;
    create_path_string(&old, oldpath);
    path_string new;
    create_path_string(&new, newpath);
    return fs_rename(&old, &new);
}

static int fdo_symlink(const char* linkname, const char* path) {
    printf("fdo_symlink unimplemented\n");
    return -ENOSYS;
}

static int fdo_link(const char* oldpath, const char* newpath) {
    printf("fdo_link unimplemented\n");
    return -ENOSYS;
}

static int fdo_open(const char* path, struct fuse_file_info* fi) {
    mode_t mode = (mode_t)fi->flags;
    fs_item* item;
    path_string p_string;
    create_path_string(&p_string, path);
    int ret = fs_access(&p_string, mode, &item);
    if (ret != 0)
        return ret;

    fi->fh = fs_fh_file_handle(item);

    // TODO: force files being open when writing etc.
    //       how many times can you open the same file before closing it?
    // TODO: check access modes when implementing file permissions
    // see https://libfuse.github.io/doxygen/structfuse__operations.html#a14b98c3f7ab97cc2ef8f9b1d9dc0709d
    return 0;
}

static int fdo_release(const char* path, struct fuse_file_info* fi) {
    fs_fh_release_file(fi->fh);
    return 0;
}

static int fdo_fsync(const char* path, int datasync, struct fuse_file_info* fi) {
    printf("fdo_fsync unimplemented\n");
    return -ENOSYS;
}

static int fdo_flush(const char* path, struct fuse_file_info* fi) {
    // our filesystem doesn't support flush so always return 0 for success
    return 0;
}

/**
 * Both statfs and statvfs syscalls call this. Both are handeled accordingly
 */
static int fdo_statfs(const char* path, struct statvfs* buf) {
    // FUSE makes sure that the file/folder is part of our fs
    return fs_statvfs(NULL, buf);
}

static int fdo_opendir(const char* path, struct fuse_file_info* fi) {
    mode_t mode = (mode_t)fi->flags;
    fs_item* item;
    path_string p_string;
    create_path_string(&p_string, path);
    int ret = fs_access(&p_string, mode, &item);
    if (ret != 0)
        return ret;

    fi->fh = fs_fh_file_handle(item);
    return 0;
}

static int fdo_releasedir(const char* path, struct fuse_file_info* fi) {
    fs_fh_release_file(fi->fh);
    return 0;
}

static int fdo_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi) {
    printf("fdo_fsyncdir unimplemented\n");
    return -ENOSYS;
}

static int fdo_chmod(const char* path, mode_t mode, struct fuse_file_info* fi) {
    path_string p_string;
    if (fi != NULL) {
        return fs_fchmod(fi->fh, mode);
    } else {
        create_path_string(&p_string, path);
        return fs_chmod(&p_string, mode);
    }
}

static int fdo_chown(const char* path, uid_t uid, gid_t gid, struct fuse_file_info* fi) {
    path_string p_string;
    if (fi != NULL) {
        return fs_fchown(fi->fh, uid, gid);
    } else {
        create_path_string(&p_string, path);
        return fs_chown(&p_string, uid, gid);
    }
}

static int fdo_utimens(const char* path, const struct timespec tv[2], struct fuse_file_info* fi) {
    // TODO: update fs_item's stat, check docs what whe tv should affect
    // we don't care times for now so just no-op
    return 0;
}

static int fdo_access(const char* path, int mask) {
    // TODO: actual file perms
    return 0;
}

static int fdo_readlink(const char* path, char* buf, size_t len) {
    printf("fdo_readlink unimplemented\n");
    return -ENOSYS;
}

static int fdo_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi, unsigned int flags, void* data) {
    printf("fdo_ioctl unimplemented\n");
    return -ENOSYS;
}

static int fdo_poll(const char* path, struct fuse_file_info* fi, struct fuse_pollhandle* ph, unsigned* reventsp) {
    printf("fdo_poll unimplemented\n");
    return -ENOSYS;
}

static int fdo_fallocate(const char* path, int mode, off_t offset, off_t length, struct fuse_file_info* fi) {
    printf("fdo_fallocate unimplemented\n");
    return -ENOSYS;
}

int main(int argc, char* argv[]) {
    // TODO: try to create the directory that's given as an arg
    int ret = 0;
    init_fs();
    // TODO: by default fuse uses umask 0022 so file mode cannot be 777 etc
    //       manually set umask to be 0000 so user can define the file mode freely
    ret = fuse_main(argc, argv, &operations, NULL);
    free_fs();
    return ret;
}
