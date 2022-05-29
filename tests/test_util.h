#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <check.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#define __USE_ATFILE
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

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
        qsort(names, nlen, 256, sstrcmp);              \
        qsort(expected, nlen, sizeof(char*), vstrcmp); \
        for (size_t ii = 0; ii < nlen; ii++)           \
            ck_assert_str_eq(names[ii], expected[ii]); \
    } while (0)
#define test_readdirh(_path, ...) test_readdir(_path, ".", "..", __VA_ARGS__)

// no-op function to make sure that a macro null terminated __VA_ARGS__
void check_null_term(char* _ptr, ...) __attribute__((sentinel(0)));
void check_null_term(char* _ptr, ...) { (void)_ptr; }
// sstrcmp is for char foo[][] and vstrcmp is for char* bar[]
int sstrcmp(const void* a, const void* b) { return strcmp((char*)a, (char*)b); }
int vstrcmp(const void* a, const void* b) { return strcmp(*(char**)a, *(char**)b); }

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

#endif
