// TEST_SYSCALLS: unlink unlinkat rmdir
/**
 * Tests for read unlink unlinkat syscalls
 * https://man7.org/linux/man-pages/man2/unlink.2.html
 * Tests for rmdir syscall
 * https://man7.org/linux/man-pages/man2/rmdir.2.html
 */

#include "test_util.h"

START_TEST(unlink_success) {
    test_readdirh(FS_PATH "unlink", "fooat.txt", "foo.txt", "no-rm", "rmdir", "unlink", "unlinkat", NULL);
    ck_assert_int_eq(unlink(FS_PATH "unlink/foo.txt"), 0);
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", "unlinkat", NULL);
    // TODO: remove symlink
}
END_TEST

START_TEST(unlink_errors) {
    // TODO: EACCESS
    // TODO: cd to FS_PATH and try to rm "." and ".."
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", "unlinkat", NULL);
    fn_errno(unlink(FS_PATH "unlink"), EISDIR);
    fn_errno(unlink(FS_PATH "unlink/nonexsiting/foo.txt"), ENOENT);
    fn_errno(unlink((void*)0x1), EFAULT);
    fn_errno(unlink(LONG_PATH), ENAMETOOLONG);
    fn_errno(unlink(LONG_NAME), ENAMETOOLONG);
    // TODO: ELOOP when adding symlinks
    // TODO: EPERM when S_ISVTX is set
    // TODO: EPERM when the file to be unlinked is marked immutable or append-only. (See ioctl_iflags(2).)
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", "unlinkat", NULL);
}
END_TEST

START_TEST(unlinkat_success) {
    int fd = open(FS_PATH "unlink", O_RDONLY);
    ck_assert_int_ge(fd, 2);
    test_readdirh(FS_PATH "unlink/unlinkat", "zar.txt", "bar2.txt", NULL);
    ck_assert_int_eq(unlinkat(fd, "unlinkat/zar.txt", 0), 0);
    test_readdirh(FS_PATH "unlink/unlinkat", "bar2.txt", NULL);
    // Make sure that rm fails with non empty but works after the last file is removed
    fn_errno(unlinkat(fd, FS_PATH "unlink/unlinkat", AT_REMOVEDIR), ENOTEMPTY);
    ck_assert_int_eq(unlinkat(-1, FS_PATH "unlink/unlinkat/bar2.txt", 0), 0);
    ck_assert_int_eq(unlinkat(fd, "unlinkat", AT_REMOVEDIR), 0);
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", NULL);
    close(fd);
}
END_TEST

START_TEST(unlinkat_errors) {
    // TODO: EACCESS
    // TODO: cd to FS_PATH and try to rm "." and ".."
    int fd = open(FS_PATH "unlink", O_RDONLY);
    ck_assert_int_ge(fd, 2);
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", NULL);
    fn_errno(unlinkat(fd, "no-rm", 0), EISDIR);
    fn_errno(unlinkat(fd, "no-rm/bar.txt", AT_REMOVEDIR), ENOTDIR);
    fn_errno(unlinkat(fd, "nonexsiting/foo.txt", 0), ENOENT);
    fn_errno(unlinkat(fd, (void*)0x1, 0), EFAULT);
    fn_errno(unlinkat(fd, LONG_PATH, 0), ENAMETOOLONG);
    fn_errno(unlinkat(fd, LONG_NAME, 0), ENAMETOOLONG);
    fn_errno(unlinkat(fd, "no-rm", AT_REMOVEDIR), ENOTEMPTY);
    // TODO: ELOOP when adding symlinks
    // TODO: EPERM when S_ISVTX is set
    // TODO: EPERM when the file to be unlinked is marked immutable or append-only. (See ioctl_iflags(2).)
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", NULL);
    close(fd);
}
END_TEST

START_TEST(rmdir_success) {
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "rmdir", "unlink", NULL);
    // Make sure that rm fails with non empty but works after the last file is removed
    fn_errno(rmdir(FS_PATH "unlink/rmdir"), ENOTEMPTY);
    ck_assert_int_eq(unlinkat(-1, FS_PATH "unlink/rmdir/har.txt", 0), 0);
    ck_assert_int_eq(rmdir(FS_PATH "unlink/rmdir"), 0);
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "unlink", NULL);
}
END_TEST

START_TEST(rmdir_errors) {
    // TODO: EACCESS
    // TODO: cd to FS_PATH and try to rm "." and ".."
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "unlink", NULL);
    fn_errno(rmdir(FS_PATH "unlink/no-rm/bar.txt"), ENOTDIR);
    fn_errno(rmdir(FS_PATH "unlink/nonexsiting/foo.txt"), ENOENT);
    fn_errno(rmdir((void*)0x1), EFAULT);
    fn_errno(rmdir(LONG_PATH), ENAMETOOLONG);
    fn_errno(rmdir(LONG_NAME), ENAMETOOLONG);
    fn_errno(rmdir(FS_PATH "unlink/no-rm"), ENOTEMPTY);
    // pathname has .  as last component.
    fn_errno(rmdir(FS_PATH "unlink/no-rm/."), EINVAL);
    // TODO: ELOOP when adding symlinks
    // TODO: EPERM when S_ISVTX is set
    // TODO: EPERM when the file to be unlinked is marked immutable or append-only. (See ioctl_iflags(2).)
    test_readdirh(FS_PATH "unlink", "fooat.txt", "no-rm", "unlink", NULL);
}
END_TEST

Suite* unlink_suite() {
    Suite* s;
    TCase* tc_core;

    // first suite needs to have "\n " to make the output cleaner
    s = suite_create("\n POSIX unlink");
    tc_core = tcase_create("POSIX unlink Core");
    tcase_add_test(tc_core, unlink_success);
    tcase_add_test(tc_core, unlink_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

Suite* unlinkat_suite() {
    Suite* s;
    TCase* tc_core;

    s = suite_create("POSIX unlinkat");
    tc_core = tcase_create("POSIX unlinkat Core");
    tcase_add_test(tc_core, unlinkat_success);
    tcase_add_test(tc_core, unlinkat_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

Suite* rmdir_suite() {
    Suite* s;
    TCase* tc_core;

    s = suite_create("POSIX rmdir");
    tc_core = tcase_create("POSIX rmdir Core");
    tcase_add_test(tc_core, rmdir_success);
    tcase_add_test(tc_core, rmdir_errors);
    suite_add_tcase(s, tc_core);

    return s;
}

int main() {
    int number_failed;
    Suite* s;
    SRunner* sr;

    s = unlink_suite();
    sr = srunner_create(s);
    srunner_add_suite(sr, unlinkat_suite());
    srunner_add_suite(sr, rmdir_suite());

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
