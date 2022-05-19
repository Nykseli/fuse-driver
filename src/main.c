
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
static int fdo_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi);
static int fdo_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* info);
static int fdo_truncate(const char* path, off_t size);
static int fdo_unlink(const char* path);
static int fdo_rmdir(const char* path);
static int fdo_rename(const char* oldpath, const char* newpath);
static int fdo_symlink(const char* linkname, const char* path);
static int fdo_link(const char* oldpath, const char* newpath);
static int fdo_open(const char* path, struct fuse_file_info* fi);
static int fdo_fsync(const char* path, int datasync, struct fuse_file_info* fi);
static int fdo_flush(const char* path, struct fuse_file_info* fi);
static int fdo_statfs(const char* path, struct statvfs* buf);
static int fdo_opendir(const char* path, struct fuse_file_info* fi);
static int fdo_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi);
static int fdo_chmod(const char* path, mode_t mode);
static int fdo_chown(const char* path, uid_t uid, gid_t gid);
static int fdo_utimens(const char* path, const struct timespec tv[2]);
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
    .readdir = fdo_readdir,
    .fsyncdir = fdo_fsyncdir,
    .access = fdo_access,
    .utimens = fdo_utimens,
    .poll = fdo_poll,
    .fallocate = fdo_fallocate,
    .ioctl = fdo_ioctl,
    // we don't have any xattrs
    // .setxattr = fdo_setxattr,
    // .getxattr = fdo_getxattr,
    // .listxattr = fdo_listxattr,
    // .removexattr = fdo_removexattr,
    // release gets called after when there is no refs and we don't care about that for now
    // .release = fdo_release,
    // .releasedir = fdo_releasedir,
    // mknod can handdle the create calls
    // .create = fdo_create,
    // let kernel handle the locks
    // .lock = fdo_lock,
    // .flock = fdo_flock,
    // let normal writes and reads handle read/write
    // .write_buf = fdo_write_buf,
    // .read_buf = fdo_read_buf,
};

static int fdo_getattr(const char* path, struct stat* st) {
    path_string p_string;
    create_path_string(&p_string, path);

    st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now

    fs_item* item;
    int ret = fs_get_item(&p_string, &item, 0);
    if (ret != 0)
        return ret;

    if (fs_item_is_dir(item)) {
        // TODO: set the mode while creating the directory
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
    } else if (fs_item_is_file(item)) {
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
    path_string p_string;
    create_path_string(&p_string, path);

    filler(buffer, ".", NULL, 0); // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    fs_dir* root;
    int ret = fs_get_directory(&p_string, &root, 0);
    if (ret != 0) {
        return ret;
    }

    const char* key;
    // TODO: filler with stat
    fs_foreach_key(&root->items, key) {
        filler(buffer, key, NULL, 0);
    }
    return 0;
}

static int fdo_mkdir(const char* path, mode_t mode) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_add_item(&p_string, FS_DIR);
}

static int fdo_mknod(const char* path, mode_t mode, dev_t rdev) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_add_item(&p_string, FS_FILE);
}

static int fdo_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_file_read(&p_string, buffer, size, offset);
}

static int fdo_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* info) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_file_write(&p_string, buffer, size, offset);
}

static int fdo_truncate(const char* path, off_t size) {
    path_string p_string;
    create_path_string(&p_string, path);
    return fs_file_truncate(&p_string, size);
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

static int fdo_rename(const char* oldpath, const char* newpath) {
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
    // TODO: check access modes when implementing file permissions
    // see https://libfuse.github.io/doxygen/structfuse__operations.html#a14b98c3f7ab97cc2ef8f9b1d9dc0709d
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

static int fdo_statfs(const char* path, struct statvfs* buf) {
    printf("fdo_statfs unimplemented\n");
    return -ENOSYS;
}

static int fdo_opendir(const char* path, struct fuse_file_info* fi) {
    // TODO: check perms when file perms get implemented
    return 0;
}

static int fdo_fsyncdir(const char* path, int datasync, struct fuse_file_info* fi) {
    printf("fdo_fsyncdir unimplemented\n");
    return -ENOSYS;
}

static int fdo_chmod(const char* path, mode_t mode) {
    printf("fdo_chmod unimplemented\n");
    return -ENOSYS;
}

static int fdo_chown(const char* path, uid_t uid, gid_t gid) {
    printf("fdo_chown unimplemented\n");
    return -ENOSYS;
}

static int fdo_utimens(const char* path, const struct timespec tv[2]) {
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
    int ret = 0;
    init_fs();
    ret = fuse_main(argc, argv, &operations, NULL);
    free_fs();
    return ret;
}
