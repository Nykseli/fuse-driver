#ifndef UTIL_H
#define UTIL_H

#define FUSE_USE_VERSION 30

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>

typedef uint64_t file_handle;

// TODO: better check if compiler supports the nonzero attribute
#ifdef NONZERO_COMPILER
#define __nonzero(params) __attribute__((__nonzero__ params))
#else
#define __nonzero(params)
#endif

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

void sleep_ms(int milliseconds);

#endif
