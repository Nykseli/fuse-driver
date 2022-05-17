#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

#define PATH_LEN_MAX 4096

#ifndef S_IFDIR
#define S_IFDIR __S_IFDIR // Directory.
#define S_IFCHR __S_IFCHR // Character device.
#define S_IFBLK __S_IFBLK // Block device.
#define S_IFREG __S_IFREG // Regular file.
#define S_IFIFO __S_IFIFO // FIFO.
#define S_IFLNK __S_IFLNK // Symbolic link.
#define S_IFSOCK __S_IFSOCK // Socket.
#endif

#endif
