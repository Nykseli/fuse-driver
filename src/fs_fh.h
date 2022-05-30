#ifndef FS_FH_H
#define FS_FH_H

#include <stdint.h>

#include "fs.h"
#include "util.h"

file_handle fs_fh_file_handle(const fs_item* item) __nonnull((1));
void fs_fh_release_file(file_handle fh) __nonzero();
int fs_fh_get_item(file_handle fh, fs_item** buf) __nonzero((1)) __nonnull((2));
int fs_fh_get_dir(file_handle fh, fs_dir** buf) __nonzero((1)) __nonnull((2));
int fs_fh_get_file(file_handle fh, fs_file** buf) __nonzero((1)) __nonnull((2));
void init_fs_fh();
void free_fs_fh();

#endif
