
#define _POSIX_C_SOURCE 200809L

#include "metadata.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


/*
 * So sánh hai mốc thời gian.
 */
static int same_timespec(
    const struct timespec *a,
    const struct timespec *b)
{
    return a->tv_sec == b->tv_sec &&
           a->tv_nsec == b->tv_nsec;
}


/*
 * So sánh toàn bộ metadata với struct stat
 * do Linux cung cấp.
 */
static int compare_metadata(
    const file_metadata_t *actual,
    const struct stat *expected)
{
#define CHECK_FIELD(field, stat_field)                 \
    do {                                               \
        if (actual->field != expected->stat_field) {   \
            fprintf(stderr,                            \
                "[FAIL] Metadata mismatch: %s\n",      \
                #field);                               \
            return -1;                                 \
        }                                              \
    } while (0)

    CHECK_FIELD(device_id, st_dev);
    CHECK_FIELD(inode_number, st_ino);
    CHECK_FIELD(size_bytes, st_size);
    CHECK_FIELD(allocated_blocks_512, st_blocks);
    CHECK_FIELD(preferred_io_block_size, st_blksize);
    CHECK_FIELD(link_count, st_nlink);
    CHECK_FIELD(mode, st_mode);
    CHECK_FIELD(uid, st_uid);
    CHECK_FIELD(gid, st_gid);

#undef CHECK_FIELD

    if (!same_timespec(
            &actual->atime,
            &expected->st_atim)) {

        fprintf(stderr, "[FAIL] Access time mismatch\n");
        return -1;
    }

    if (!same_timespec(
            &actual->mtime,
            &expected->st_mtim)) {

        fprintf(stderr, "[FAIL] Modify time mismatch\n");
        return -1;
    }

    if (!same_timespec(
            &actual->ctime,
            &expected->st_ctim)) {

        fprintf(stderr, "[FAIL] Change time mismatch\n");
        return -1;
    }

    return 0;
}


/*
 * Kiểm tra các trường hợp truyền NULL.
 */
static int test_invalid_arguments(
    const char *path,
    const file_metadata_t *metadata)
{
    file_metadata_t temporary;

    FILE *stream = tmpfile();

    if (stream == NULL) {
        perror("tmpfile");
        return -1;
    }

    errno = 0;

    if (metadata_read(NULL, &temporary) != -1 ||
        errno != EINVAL) {

        fprintf(stderr, "[FAIL] metadata_read NULL path\n");
        fclose(stream);
        return -1;
    }

    errno = 0;

    if (metadata_read(path, NULL) != -1 ||
        errno != EINVAL) {

        fprintf(stderr, "[FAIL] metadata_read NULL output\n");
        fclose(stream);
        return -1;
    }

    errno = 0;

    if (metadata_print(NULL, path, metadata) != -1 ||
        errno != EINVAL) {

        fprintf(stderr, "[FAIL] metadata_print NULL stream\n");
        fclose(stream);
        return -1;
    }

    errno = 0;

    if (metadata_print(stream, NULL, metadata) != -1 ||
        errno != EINVAL) {

        fprintf(stderr, "[FAIL] metadata_print NULL path\n");
        fclose(stream);
        return -1;
    }

    errno = 0;

    if (metadata_print(stream, path, NULL) != -1 ||
        errno != EINVAL) {

        fprintf(stderr, "[FAIL] metadata_print NULL metadata\n");
        fclose(stream);
        return -1;
    }

    fclose(stream);

    return 0;
}


/*
 * Kiểm tra khả năng xuất metadata thành công.
 */
static int test_print_success(
    const char *path,
    const file_metadata_t *metadata)
{
    FILE *stream = tmpfile();

    if (stream == NULL) {
        perror("tmpfile");
        return -1;
    }

    if (metadata_print(stream, path, metadata) == -1) {
        perror("metadata_print");
        fclose(stream);
        return -1;
    }

    if (fflush(stream) == EOF) {
        perror("fflush");
        fclose(stream);
        return -1;
    }

    if (fseek(stream, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(stream);
        return -1;
    }

    long output_size = ftell(stream);

    fclose(stream);

    if (output_size <= 0) {
        fprintf(stderr, "[FAIL] Empty metadata output\n");
        return -1;
    }

    return 0;
}


/*
 * Kiểm tra lỗi khi luồng xuất không ghi được.
 * /dev/full là thiết bị Linux luôn báo hết dung lượng.
 */
static int test_print_error(
    const char *path,
    const file_metadata_t *metadata)
{
    FILE *stream = fopen("/dev/full", "w");

    if (stream == NULL) {
        printf("[SKIP] /dev/full unavailable\n");
        return 0;
    }

    if (setvbuf(stream, NULL, _IONBF, 0) != 0) {
        fclose(stream);
        fprintf(stderr, "[FAIL] Cannot configure output stream\n");
        return -1;
    }

    errno = 0;

    int result = metadata_print(stream, path, metadata);

    fclose(stream);

    if (result != -1) {
        fprintf(stderr, "[FAIL] Output error was not detected\n");
        return -1;
    }

    return 0;
}


int main(void)
{
    char path[] = "/tmp/metadata_api_XXXXXX";

    int fd = mkstemp(path);

    if (fd == -1) {
        perror("mkstemp");
        return EXIT_FAILURE;
    }

    int exit_status = EXIT_FAILURE;

    /*
     * Tạo nội dung file thử nghiệm.
     */
    const char content[] = "Metadata API comparison test\n";

    size_t length = sizeof(content) - 1;
    size_t offset = 0;

    while (offset < length) {

        ssize_t written = write(
            fd,
            content + offset,
            length - offset
        );

        if (written < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("write");
            goto cleanup;
        }

        if (written == 0) {
            fprintf(stderr, "[FAIL] Write returned zero\n");
            goto cleanup;
        }

        offset += (size_t)written;
    }

    /*
     * Đặt quyền file cố định để dễ kiểm tra.
     */
    if (fchmod(fd, 0640) == -1) {
        perror("fchmod");
        goto cleanup;
    }

    if (close(fd) == -1) {
        fd = -1;
        perror("close");
        goto cleanup;
    }

    fd = -1;

    /*
     * Đọc metadata tham chiếu bằng stat().
     */
    struct stat expected;

    if (stat(path, &expected) == -1) {
        perror("stat");
        goto cleanup;
    }

    /*
     * Đọc metadata bằng module đang phát triển.
     */
    file_metadata_t actual;

    if (metadata_read(path, &actual) == -1) {
        perror("metadata_read");
        goto cleanup;
    }

    /*
     * So sánh toàn bộ trường metadata.
     */
    if (compare_metadata(&actual, &expected) == -1) {
        goto cleanup;
    }

    printf("[PASS] All metadata fields match stat()\n");

    /*
     * Kiểm tra tham số NULL.
     */
    if (test_invalid_arguments(path, &actual) == -1) {
        goto cleanup;
    }

    printf("[PASS] NULL argument validation\n");

    /*
     * Kiểm tra xuất dữ liệu thành công.
     */
    if (test_print_success(path, &actual) == -1) {
        goto cleanup;
    }

    printf("[PASS] Metadata output\n");

    /*
     * Kiểm tra lỗi luồng xuất.
     */
    if (test_print_error(path, &actual) == -1) {
        goto cleanup;
    }

    if (access("/dev/full", F_OK) == 0) {
        printf("[PASS] Output error handling\n");
    }

    exit_status = EXIT_SUCCESS;

cleanup:

    if (fd >= 0) {
        close(fd);
    }

    unlink(path);

    return exit_status;
}
