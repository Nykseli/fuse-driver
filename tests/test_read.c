// TEST_SYSCALLS: read readdir
/**
 * Tests for read syscall
 * https://man7.org/linux/man-pages/man2/read.2.html
 * Tests for readdir (getdets) syscall
 * https://man7.org/linux/man-pages/man3/readdir.3.html
 * https://man7.org/linux/man-pages/man2/getdents.2.html
 */

#include "test_util.h"

START_TEST(read_success) {
    // TODO: make sure symbolic and hardlinks work
    char foobuf[5];
    foobuf[4] = '\0';
    char bigbuf[256];
    bigbuf[4] = '\0';
    int fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    ck_assert_uint_eq(read(fd, foobuf, 4), 4);
    fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    ck_assert_uint_eq(read(fd, foobuf, 256), 4);
    ck_assert_str_eq("foo\n", foobuf);
    close(fd);
    fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    ck_assert_uint_eq(read(fd, bigbuf, 256), 4);
    ck_assert_str_eq("foo\n", bigbuf);
    ck_assert_uint_eq(read(fd, bigbuf, 256), 0);
    close(fd);
    fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    foobuf[2] = 'A';
    ck_assert_uint_eq(read(fd, foobuf, 2), 2);
    ck_assert_uint_eq(read(fd, foobuf + 2, 2), 2);
    ck_assert_str_eq("foo\n", foobuf);
    close(fd);
}
END_TEST

START_TEST(read_errors) {
    // TODO: EAGAIN
    char err[1];
    fn_errno(read(-1, err, 1), EBADF); // file not found
    int fd = open(FS_PATH "foo_file.txt", O_WRONLY);
    ck_assert_uint_ge(fd, 2);
    fn_errno(read(fd, err, 1), EBADF);
    close(fd);
    fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    ck_assert_uint_ge(fd, 2);
    close(fd);
    fn_errno(read(fd, err, 1), EBADF);
    fd = open(FS_PATH "foo_file.txt", O_RDONLY);
    fn_errno(read(fd, NULL, 1), EFAULT);
    // TODO: EINVAL
    // TODO: EIO
    fd = open(FS_PATH, O_RDONLY);
    fn_errno(read(fd, NULL, 1), EISDIR);
}
END_TEST

START_TEST(readdir_success) {
    // TODO: make sure symbolic and hardlinks work
    // TODO: check struct dirent d_type
    test_readdir(FS_PATH "read_dir", ".", "..", "foo.txt", "nest1", "empty.txt");
    test_readdir(FS_PATH "read_dir/nest1", ".", "..", "nest2", "bar.txt");
    test_readdir(FS_PATH "read_dir/nest1/nest2", ".", "..", "zar.txt");
}
END_TEST

START_TEST(readdir_errors) {
    DIR* dh = opendir(FS_PATH "read_dir");
    closedir(dh);
    struct dirent* dent = readdir(dh);
    ck_assert_int_eq(errno, EBADF);
    ck_assert_ptr_null(dent);
}
END_TEST

Suite* read_suite() {
    Suite* s;
    TCase* tc_core;

    // first suite needs to have "\n " to make the output cleaner
    s = suite_create("\n POSIX read");
    tc_core = tcase_create("POSIX read Core");
    tcase_add_test(tc_core, read_success);
    tcase_add_test(tc_core, read_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

Suite* readdir_suite() {
    Suite* s;
    TCase* tc_core;

    // https://iq.opengenus.org/ls-command-in-c/
    s = suite_create("POSIX readdir");
    tc_core = tcase_create("POSIX readdir Core");
    tcase_add_test(tc_core, readdir_success);
    tcase_add_test(tc_core, readdir_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = read_suite();
    sr = srunner_create(s);
    srunner_add_suite(sr, readdir_suite());

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
