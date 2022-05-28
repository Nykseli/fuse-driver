#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <check.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define __USE_ATFILE
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "../src/fs.h"
#include "../src/util.h"

#define FS_PATH "/tmp/fuse_test/"

#define DEF_DIR_MODE S_IFDIR | 0755
#define DEF_FILE_MODE S_IFREG | 0644

#define fn_errno(_fn, _err)    \
    ck_assert_int_eq(_fn, -1); \
    ck_assert_int_eq(errno, _err)

// test_readdir(FS_PATH "read_dir", ".", "..", "foo.txt", "nest1", "empty.txt", NULL);
#define test_readdir(_path, ...)                       \
    do {                                               \
        check_null_term(_path, __VA_ARGS__);           \
        struct dirent* dent;                           \
        DIR* dh = opendir(_path);                      \
        ck_assert_ptr_nonnull(dh);                     \
        char names[256][FILE_NAME_MAX + 1];            \
        char* expected[256] = { __VA_ARGS__ };         \
        size_t explen = 0;                             \
        while (expected[explen] != NULL)               \
            explen++;                                  \
        ck_assert_int_lt(explen, 256);                 \
        size_t nlen = 0;                               \
        while ((dent = readdir(dh)) != NULL) {         \
            strcpy(names[nlen], dent->d_name);         \
            nlen++;                                    \
        }                                              \
        ck_assert_int_eq(explen, nlen);                \
        for (size_t ii = 0; ii < nlen; ii++)           \
            ck_assert_str_eq(names[ii], expected[ii]); \
    } while (0)

// no-op function to make sure that a macro null terminated __VA_ARGS__
void check_null_term(char* _ptr, ...) __attribute__((sentinel(0)));
void check_null_term(char* _ptr, ...) { (void)_ptr; }

#endif
