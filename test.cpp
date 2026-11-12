
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat info;

    if (stat(argv[1], &info) == -1) {
        perror("stat");
        return EXIT_FAILURE;
    }

    printf("===== FILE METADATA =====\n");
    printf("File: %s\n", argv[1]);
    printf("Inode: %ju\n", (uintmax_t)info.st_ino);
    printf("Size: %jd bytes\n", (intmax_t)info.st_size);
    printf("Mode: %jo\n", (uintmax_t)info.st_mode);
    printf("UID: %ju\n", (uintmax_t)info.st_uid);
    printf("GID: %ju\n", (uintmax_t)info.st_gid);
    printf("Block size: %jd bytes\n", (intmax_t)info.st_blksize);
    printf("Allocated blocks: %jd\n", (intmax_t)info.st_blocks);

    return EXIT_SUCCESS;
}
