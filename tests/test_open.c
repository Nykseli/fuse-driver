// TEST_SYSCALLS: open openat creat
/**
 * Tests for open syscall
 * https://man7.org/linux/man-pages/man2/open.2.html
 */

#include "test_util.h"

#define LONG_PATH gen_path(4095, false)
#define LONG_NAME gen_path(256, true)

char* gen_path(size_t len, bool txt) {
    size_t path_len = strlen(FS_PATH);
    path_len += len;
    if (txt)
        len += 4;
    char* path = malloc(path_len + 1);
    memcpy(path, FS_PATH, 15);
    for (size_t ii = 15; ii < path_len; ii++)
        path[ii] = 'A';
    if (txt)
        memcpy(path + (path_len - 4), ".txt", 4);
    path[path_len] = '\0';
    return path;
}

#define stat_equal(_fpath, _st1, _st2)                     \
    ck_assert_int_eq(stat(_fpath, _st2), 0);               \
    ck_assert_uint_eq((_st1)->st_uid, (_st2)->st_uid);     \
    ck_assert_uint_eq((_st1)->st_gid, (_st2)->st_gid);     \
    ck_assert_uint_eq((_st1)->st_size, (_st2)->st_size);   \
    ck_assert_uint_eq((_st1)->st_mode, (_st2)->st_mode);   \
    ck_assert_uint_eq((_st1)->st_nlink, (_st2)->st_nlink); \
    ck_assert_uint_eq((_st1)->st_blocks, (_st2)->st_blocks)

#define statat_equal(_dirfd, _fpath, _st1, _st2)           \
    ck_assert_int_eq(fstatat(_dirfd, _fpath, _st2, 0), 0); \
    ck_assert_uint_eq((_st1)->st_uid, (_st2)->st_uid);     \
    ck_assert_uint_eq((_st1)->st_gid, (_st2)->st_gid);     \
    ck_assert_uint_eq((_st1)->st_size, (_st2)->st_size);   \
    ck_assert_uint_eq((_st1)->st_mode, (_st2)->st_mode);   \
    ck_assert_uint_eq((_st1)->st_nlink, (_st2)->st_nlink); \
    ck_assert_uint_eq((_st1)->st_blocks, (_st2)->st_blocks)

#define open_errno(_fn, _err)  \
    ck_assert_int_eq(_fn, -1); \
    ck_assert_int_eq(errno, _err)

void stat_init(struct stat* st) {
    // copy from src/fs.c
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    st->st_ctime = time(NULL);
    st->st_size = 0;
    st->st_blksize = FS_BLOCK_SIZE;
    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 1;
    st->st_ino = 0;
    st->st_dev = 0;
    st->st_blocks = 0;
}

START_TEST(open_success) {
    struct stat st1, st2;
    stat_init(&st1);
    ck_assert_int_ge(open(FS_PATH "o_creat_text.txt", O_RDWR | O_CREAT, S_IFREG | 0644), 2);
    stat_equal(FS_PATH "o_creat_text.txt", &st1, &st2);
    // TODO: support 0777, currently the fuse umask will remove 022 so 777 -> 755
    // st1.st_mode = S_IFREG | 0777;
    // ck_assert_int_ge(open(FS_PATH"./o_creat_text777.txt", O_RDWR | O_CREAT, S_IFREG | 0777), 2);
    // stat_equal(FS_PATH"./o_creat_text777.txt", &st1, &st2);
    // TODO: grep and all mode tests
    st1.st_mode = S_IFREG | 0755;
    ck_assert_int_ge(open(FS_PATH "./o_creat_text777.txt", O_RDWR | O_CREAT, S_IFREG | 0755), 2);
    stat_equal(FS_PATH "./o_creat_text777.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0400;
    ck_assert_int_ge(open(FS_PATH "test_dir/../test_dir/o_creat_text_rel.txt", O_RDONLY | O_CREAT, S_IFREG | 0400), 2);
    stat_equal(FS_PATH "test_dir/../test_dir/o_creat_text_rel.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0644;
    st1.st_size = 4;
    ck_assert_int_ge(open(FS_PATH "foo_file.txt", O_RDWR | O_CREAT), 2);
    stat_equal(FS_PATH "foo_file.txt", &st1, &st2);
    // opening a directory
    st1.st_size = 4096;
    st1.st_mode = S_IFDIR | 0755;
    st1.st_nlink = 2;
    ck_assert_int_ge(open(FS_PATH, O_RDONLY), 2);
    stat_equal(FS_PATH, &st1, &st2);
}
END_TEST

START_TEST(open_errors) {
    open_errno(open(FS_PATH "norw_file.txt", O_RDWR), EACCES);
    open_errno(open(FS_PATH "norw_file.txt", O_WRONLY), EACCES);
    open_errno(open(FS_PATH "norw_file.txt", O_RDONLY), EACCES);
    // EBADF not related to open()
    // EBUSY irrelevant on our file system
    // EDQUOT irrelevant on our file system
    open_errno(open(FS_PATH "foo_file.txt", O_RDWR | O_CREAT | O_EXCL), EEXIST);
    // EFAULT irrelevant on our file system
    // EFBIG irrelevant on our file system
    // TODO: EINTR
    // TODO: EINVAL (see all the possible cases from the man page)
    open_errno(open(FS_PATH "tmp.txt", O_RDWR | O_CREAT | __O_TMPFILE, S_IFREG | 0644), EINVAL);
    open_errno(open(FS_PATH, O_RDWR), EISDIR);
    // TODO: ELOOP when adding symlink support
    // TODO: EMFILE
    open_errno(open(LONG_PATH, O_RDWR), ENAMETOOLONG);
    open_errno(open(LONG_NAME, O_RDWR), ENAMETOOLONG);
    // TODO: ENFILE
    // TODO: ENODEV
    // TODO: ENOENT A directory component in pathname does not exist or is a
    //              dangling symbolic link.
    open_errno(open(FS_PATH "non_existing.txt", O_RDONLY), ENOENT);
    // TODO: ENOMEM when fifo support is added
    // TODO: ENOSPC
    open_errno(open(FS_PATH "foo_file.txt", O_RDONLY | __O_DIRECTORY), ENOTDIR);
    // TODO: ENXIO fifo support
    // EOPNOTSUPP we don't support tmp files and fuse seems to give EINVAL error from it
    // TODO: EOVERFLOW
    // TODO: EPERM when adding file seals
    // EROFS irrelevant to our fs
    // TODOETXTBSY fuse should handle this natively
    // TODO: EWOULDBLOCK
}
END_TEST

START_TEST(openat_success) {
    struct stat st1, st2;
    stat_init(&st1);
    int fuse_dir = open(FS_PATH, O_RDONLY);
    ck_assert_int_ge(fuse_dir, 2);
    ck_assert_int_ge(openat(fuse_dir, "at_creat_text.txt", O_RDWR | O_CREAT, S_IFREG | 0644), 2);
    statat_equal(fuse_dir, "at_creat_text.txt", &st1, &st2);
    // TODO: support 0777, currently the fuse umask will remove 022 so 777 -> 755
    // st1.st_mode = S_IFREG | 0777;
    // ck_assert_int_ge(open("./at_creat_text777.txt", O_RDWR | O_CREAT, S_IFREG | 0777), 2);
    // statat_equal(fuse_dir, "./at_creat_text777.txt", &st1, &st2);
    // TODO: grep and all mode tests
    st1.st_mode = S_IFREG | 0755;
    ck_assert_int_ge(openat(fuse_dir, "./o_creat_text777.txt", O_RDWR | O_CREAT, S_IFREG | 0755), 2);
    statat_equal(fuse_dir, "./o_creat_text777.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0400;
    ck_assert_int_ge(openat(fuse_dir, "test_dir/../test_dir/o_creat_text_rel.txt", O_RDONLY | O_CREAT, S_IFREG | 0400), 2);
    statat_equal(fuse_dir, "test_dir/../test_dir/o_creat_text_rel.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0644;
    st1.st_size = 4;
    ck_assert_int_ge(openat(fuse_dir, "/tmp/fuse_test/foo_file.txt", O_RDWR | O_CREAT), 2);
    statat_equal(fuse_dir, "/tmp/fuse_test/foo_file.txt", &st1, &st2);
    // opening a directory
    st1.st_size = 4096;
    st1.st_mode = S_IFDIR | 0755;
    st1.st_nlink = 2;
    ck_assert_int_ge(openat(fuse_dir, "test_dir", O_RDONLY), 2);
    statat_equal(fuse_dir, "test_dir", &st1, &st2);
}
END_TEST

START_TEST(openat_errors) {
    int fuse_dir = open(FS_PATH, O_RDONLY);
    ck_assert_int_ge(fuse_dir, 2);

    open_errno(openat(fuse_dir, "norw_file.txt", O_RDWR), EACCES);
    open_errno(openat(fuse_dir, "norw_file.txt", O_WRONLY), EACCES);
    open_errno(openat(fuse_dir, "norw_file.txt", O_RDONLY), EACCES);
    open_errno(openat(-1, "norw_file.txt", O_RDONLY), EBADF);
    // EBUSY irrelevant on our file system
    // EDQUOT irrelevant on our file system
    open_errno(openat(fuse_dir, "foo_file.txt", O_RDWR | O_CREAT | O_EXCL), EEXIST);
    // EFAULT irrelevant on our file system
    // EFBIG irrelevant on our file system
    // TODO: EINTR
    // TODO: EINVAL (see all the possible cases from the man page)
    open_errno(openat(fuse_dir, "tmp.txt", O_RDWR | O_CREAT | __O_TMPFILE, S_IFREG | 0644), EINVAL);
    open_errno(openat(fuse_dir, "test_dir", O_RDWR), EISDIR);
    // TODO: ELOOP when adding symlink support
    // TODO: EMFILE
    open_errno(openat(fuse_dir, LONG_PATH, O_RDWR), ENAMETOOLONG);
    open_errno(openat(fuse_dir, LONG_NAME, O_RDWR), ENAMETOOLONG);
    // TODO: ENFILE
    // TODO: ENODEV
    // TODO: ENOENT A directory component in pathname does not exist or is a
    //              dangling symbolic link.
    open_errno(openat(fuse_dir, "non_existing.txt", O_RDONLY), ENOENT);
    // TODO: ENOMEM when fifo support is added
    // TODO: ENOSPC
    int foo_file = open(FS_PATH "foo_file.txt", O_RDONLY);
    ck_assert_int_ge(foo_file, 2);
    open_errno(openat(foo_file, "foo_file.txt", O_RDONLY), ENOTDIR);
    // TODO: ENXIO fifo support
    // EOPNOTSUPP we don't support tmp files and fuse seems to give EINVAL error from it
    // TODO: EOVERFLOW
    // TODO: EPERM when adding file seals
    // EROFS irrelevant to our fs
    // TODOETXTBSY fuse should handle this natively
    // TODO: EWOULDBLOCK
}
END_TEST

START_TEST(creat_success) {
    struct stat st1, st2;
    stat_init(&st1);
    ck_assert_int_ge(creat(FS_PATH "creat_creat_text.txt", S_IFREG | 0644), 2);
    stat_equal(FS_PATH "creat_creat_text.txt", &st1, &st2);
    // TODO: support 0777, currently the fuse umask will remove 022 so 777 -> 755
    // st1.st_mode = S_IFREG | 0777;
    // ck_assert_int_ge(creat(FS_PATH"./creat_creat_text777.txt", S_IFREG | 0777), 2);
    // stat_equal(FS_PATH"./creat_creat_text777.txt", &st1, &st2);
    // TODO: grep and all mode tests
    st1.st_mode = S_IFREG | 0755;
    ck_assert_int_ge(creat(FS_PATH "./creat_creat_text777.txt", S_IFREG | 0755), 2);
    stat_equal(FS_PATH "./creat_creat_text777.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0200;
    ck_assert_int_ge(creat(FS_PATH "test_dir/../test_dir/creat_creat_text_rel.txt", S_IFREG | 0200), 2);
    stat_equal(FS_PATH "test_dir/../test_dir/creat_creat_text_rel.txt", &st1, &st2);
    st1.st_mode = S_IFREG | 0644;
    ck_assert_int_ge(creat(FS_PATH "foocreat_file.txt", S_IFREG | 0400), 2);
    stat_equal(FS_PATH "foocreat_file.txt", &st1, &st2);
}
END_TEST

START_TEST(creat_errors) {
    open_errno(creat(FS_PATH "norw_file.txt", S_IFREG | 0644), EACCES);
    open_errno(creat(FS_PATH "norw_file.txt", S_IFREG | 0644), EACCES);
    open_errno(creat(FS_PATH "norw_file.txt", S_IFREG | 0644), EACCES);
    // EBADF not relevant to creat()
    // EBUSY irrelevant on our file system
    // EDQUOT irrelevant on our file system
    // EEXIST not relevant to creat()
    // EFAULT irrelevant on our file system
    // EFBIG irrelevant on our file system
    // TODO: EINTR
    // TODO: EINVAL (see all the possible cases from the man page)
    open_errno(creat(FS_PATH, S_IFREG | 0644), EISDIR);
    // TODO: ELOOP when adding symlink support
    // TODO: EMFILE
    open_errno(creat(LONG_PATH, S_IFREG | 0644), ENAMETOOLONG);
    open_errno(creat(LONG_NAME, S_IFREG | 0644), ENAMETOOLONG);
    // TODO: ENFILE
    // TODO: ENODEV
    // ENOENT not relevant to creat()
    // TODO: ENOMEM when fifo support is added
    // TODO: ENOSPC
    // ENOTDIR is not relevant to creat()
    // TODO: ENXIO fifo support
    // EOPNOTSUPP we don't support tmp files and fuse seems to give EINVAL error from it
    // TODO: EOVERFLOW
    // TODO: EPERM when adding file seals
    // EROFS irrelevant to our fs
    // TODOETXTBSY fuse should handle this natively
    // TODO: EWOULDBLOCK
}
END_TEST

Suite* open_suite() {
    Suite* s;
    TCase* tc_core;

    // first suite needs to have "\n " to make the output cleaner
    s = suite_create("\n POSIX open");
    tc_core = tcase_create("POSIX open Core");
    tcase_add_test(tc_core, open_success);
    tcase_add_test(tc_core, open_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

Suite* openat_suite() {
    Suite* s;
    TCase* tc_core;

    s = suite_create("POSIX openat");
    tc_core = tcase_create("POSIX openat Core");
    tcase_add_test(tc_core, openat_success);
    tcase_add_test(tc_core, openat_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

Suite* creat_suite() {
    Suite* s;
    TCase* tc_core;

    s = suite_create("POSIX creat");
    tc_core = tcase_create("POSIX creat Core");
    tcase_add_test(tc_core, creat_success);
    tcase_add_test(tc_core, creat_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = open_suite();
    sr = srunner_create(s);
    srunner_add_suite(sr, openat_suite());
    srunner_add_suite(sr, creat_suite());

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
