#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_FS_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_FS_H

    #include <stdint.h>

    typedef struct vittec_file_buf {
  char* data;
  uint64_t len;
} vittec_file_buf_t;

int vittec_read_entire_file(const char* path, vittec_file_buf_t* out);
void vittec_free_file_buf(vittec_file_buf_t* b);

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_FS_H */
