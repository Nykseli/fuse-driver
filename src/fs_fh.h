#ifndef FS_FH_H
#define FS_FH_H

#include <stdint.h>

#include "fs.h"
#include "util.h"

file_handle fs_fh_file_handle(fs_item* item);
void fs_fh_release_file(file_handle fh);
int fs_fh_get_item(file_handle fh, fs_item** buf);
int fs_fh_get_dir(file_handle fh, fs_dir** buf);
int fs_fh_get_file(file_handle fh, fs_file** buf);
void init_fs_fh();
void free_fs_fh();

#endif
