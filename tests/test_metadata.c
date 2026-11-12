
#define _POSIX_C_SOURCE 200809L

#include "metadata.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
    /*
     * Kiểm tra tham số đầu vào.
     */
    if (argc != 2) {

        fprintf(
            stderr,
            "Usage: %s <file_path>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }

    /*
     * Khởi tạo cấu trúc lưu metadata.
     */
    file_metadata_t metadata;

    /*
     * Gọi module đọc metadata.
     */
    if (metadata_read(argv[1], &metadata) == -1) {

        perror("metadata_read");

        return EXIT_FAILURE;
    }

    /*
     * Gọi module hiển thị kết quả.
     */
    if (metadata_print(
        stdout,
        argv[1],
        &metadata
    ) == -1) {

        perror("metadata_print");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
