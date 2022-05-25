#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <check.h>
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


#define fn_errno(_fn, _err)  \
    ck_assert_int_eq(_fn, -1); \
    ck_assert_int_eq(errno, _err)

#endif
