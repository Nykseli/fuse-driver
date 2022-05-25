// TEST_SYSCALLS:
/**
 * Tests for write syscall
 * https://man7.org/linux/man-pages/man2/write.2.html
 */

#include "test_util.h"

#define write_check(_path, _cmpstr, _strsize)                 \
    do {                                                      \
        char buf[256];                                        \
        int _fd = open(_path, O_RDONLY);                      \
        ck_assert_int_eq(read(_fd, buf, _strsize), _strsize); \
        buf[_strsize] = '\0';                                 \
        ck_assert_str_eq(_cmpstr, buf);                       \
        close(_fd);                                           \
    } while (0)

#define write_err(_path, _flag, _buff, _bufs, _err)  \
    do {                                             \
        int _fd = open(_path, _flag, DEF_FILE_MODE); \
        fn_errno(write(_fd, _buff, _bufs), _err);    \
        close(_fd);                                  \
    } while (0)

START_TEST(write_success) {
    // TODO: make sure symbolic and hardlinks work
    int fd = open(FS_PATH "write_test.txt", O_RDWR | O_CREAT, DEF_FILE_MODE);
    ck_assert_int_eq(write(fd, "FOOBAR", 6), 6);
    close(fd);
    write_check(FS_PATH "write_test.txt", "FOOBAR", 6);
    fd = open(FS_PATH "test_dir/write_test.txt", O_RDWR | O_CREAT, DEF_FILE_MODE);
    ck_assert_int_eq(write(fd, "FOOBAR", 6), 6);
    close(fd);
    write_check(FS_PATH "test_dir/write_test.txt", "FOOBAR", 6);
    fd = open(FS_PATH "test_dir/write_test2.txt", O_RDWR | O_CREAT, DEF_FILE_MODE);
    ck_assert_int_eq(write(fd, "FOO", 3), 3);
    ck_assert_int_eq(write(fd, "BAR", 3), 3);
    close(fd);
    write_check(FS_PATH "test_dir/write_test2.txt", "FOOBAR", 6);
}
END_TEST

START_TEST(write_errors) {
    // TODO: EAGAIN
    char err[1] = { 'A' };
    fn_errno(write(-1, err, 1), EBADF); // file not found
    int fd = open(FS_PATH "write_test_err.txt", O_RDONLY | O_CREAT, DEF_FILE_MODE);
    ck_assert_uint_ge(fd, 2);
    fn_errno(write(fd, err, 1), EBADF);
    close(fd);
    write_err(FS_PATH, O_RDWR, NULL, 1, EBADF); // dir
    write_err(FS_PATH "write_test_err.txt", O_WRONLY | O_CREAT, NULL, 1, EFAULT);
    // TODO: EDESTADDRREQ
    // TODO: EINVAL
    // TODO: EIO
    // TODO: EPERM
    // TODO: EPIPE
    // TODO: EINTR
}
END_TEST

Suite* write_suite() {
    Suite* s;
    TCase* tc_core;

    // first suite needs to have "\n " to make the output cleaner
    s = suite_create("\n POSIX write");
    tc_core = tcase_create("POSIX write Core");
    tcase_add_test(tc_core, write_success);
    tcase_add_test(tc_core, write_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = write_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
