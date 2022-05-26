#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <sys/stat.h>

typedef uint64_t file_handle;

// FUSE fs magic for statfs
#define FUSE_SUPER_MAGIC 0x65735546

#define PATH_LEN_MAX 4095
#define FILE_NAME_MAX 255 // 256 - space for null char

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
