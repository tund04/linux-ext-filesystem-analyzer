
#ifndef METADATA_H
#define METADATA_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {

    dev_t device_id;

    ino_t inode_number;

    off_t size_bytes;

    blkcnt_t allocated_blocks_512;

    blksize_t preferred_io_block_size;

    nlink_t link_count;

    mode_t mode;

    uid_t uid;

    gid_t gid;

    struct timespec atime;

    struct timespec mtime;

    struct timespec ctime;

} file_metadata_t;



int metadata_read(
    const char *path,
    file_metadata_t *out
);



int metadata_print(
    FILE *stream,
    const char *path,
    const file_metadata_t *metadata
);

#endif
