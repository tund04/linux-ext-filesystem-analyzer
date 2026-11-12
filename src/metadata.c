
#define _POSIX_C_SOURCE 200809L

#include "metadata.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <time.h>


/*
 * Xác định loại file dựa trên st_mode.
 */
static const char *metadata_get_file_type(mode_t mode)
{
    if (S_ISREG(mode))
        return "Regular file";

    if (S_ISDIR(mode))
        return "Directory";

    if (S_ISLNK(mode))
        return "Symbolic link";

    if (S_ISCHR(mode))
        return "Character device";

    if (S_ISBLK(mode))
        return "Block device";

    if (S_ISFIFO(mode))
        return "FIFO / Pipe";

    if (S_ISSOCK(mode))
        return "Socket";

    return "Unknown";
}


/*
 * Đọc metadata của file bằng stat().
 */
int metadata_read(
    const char *path,
    file_metadata_t *out)
{
    if (path == NULL || out == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct stat st;

    /*
     * Yêu cầu Linux đọc metadata.
     */
    if (stat(path, &st) == -1) {
        return -1;
    }

    /*
     * Lưu dữ liệu vào cấu trúc tạm.
     */
    file_metadata_t result = {0};

    result.device_id = st.st_dev;

    result.inode_number = st.st_ino;

    result.size_bytes = st.st_size;

    result.allocated_blocks_512 = st.st_blocks;

    result.preferred_io_block_size = st.st_blksize;

    result.link_count = st.st_nlink;

    result.mode = st.st_mode;

    result.uid = st.st_uid;

    result.gid = st.st_gid;

    result.atime = st.st_atim;

    result.mtime = st.st_mtim;

    result.ctime = st.st_ctim;

    /*
     * Chỉ cập nhật dữ liệu đầu ra khi đọc thành công.
     */
    *out = result;

    return 0;
}


/*
 * In thời gian theo định dạng dễ đọc.
 */
static int metadata_print_time(
    FILE *stream,
    const char *label,
    const struct timespec *value)
{
    struct tm local_time;

    char buffer[64];

    if (localtime_r(&value->tv_sec, &local_time) == NULL) {

        return fprintf(
            stream,
            "%s: %" PRIdMAX " seconds since epoch\n",
            label,
            (intmax_t)value->tv_sec
        ) < 0 ? -1 : 0;
    }

    if (strftime(
        buffer,
        sizeof(buffer),
        "%Y-%m-%d %H:%M:%S",
        &local_time
    ) == 0) {
        errno = EOVERFLOW;
        return -1;
    }

    if (fprintf(
        stream,
        "%s: %s.%09ld\n",
        label,
        buffer,
        value->tv_nsec
    ) < 0) {
        return -1;
    }

    return 0;
}


/*
 * Hiển thị metadata.
 */
int metadata_print(
    FILE *stream,
    const char *path,
    const file_metadata_t *metadata)
{
    if (stream == NULL ||
        path == NULL ||
        metadata == NULL) {

        errno = EINVAL;
        return -1;
    }

    if (fprintf(stream,
        "\n========== FILE METADATA ==========\n") < 0) {
        return -1;
    }

    if (fprintf(stream,
        "File path: %s\n", path) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "File type: %s\n",
        metadata_get_file_type(metadata->mode)) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Device ID: %u:%u\n",
        (unsigned int)major(metadata->device_id),
        (unsigned int)minor(metadata->device_id)) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Inode number: %" PRIuMAX "\n",
        (uintmax_t)metadata->inode_number) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "File size: %" PRIdMAX " bytes\n",
        (intmax_t)metadata->size_bytes) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Allocated blocks: %" PRIdMAX
        " (512-byte units)\n",
        (intmax_t)metadata->allocated_blocks_512) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Preferred I/O block size: %" PRIdMAX " bytes\n",
        (intmax_t)metadata->preferred_io_block_size) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Link count: %" PRIuMAX "\n",
        (uintmax_t)metadata->link_count) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "Permission bits: %04o\n",
        (unsigned int)(metadata->mode & 07777)) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "UID: %" PRIuMAX "\n",
        (uintmax_t)metadata->uid) < 0) {
        return -1;
    }

    if (fprintf(stream,
        "GID: %" PRIuMAX "\n",
        (uintmax_t)metadata->gid) < 0) {
        return -1;
    }

    if (metadata_print_time(
        stream, "Access time", &metadata->atime) == -1) {
        return -1;
    }

    if (metadata_print_time(
        stream, "Modify time", &metadata->mtime) == -1) {
        return -1;
    }

    if (metadata_print_time(
        stream, "Change time", &metadata->ctime) == -1) {
        return -1;
    }

    if (fprintf(stream,
        "===================================\n") < 0) {
        return -1;
    }

    return 0;
}
