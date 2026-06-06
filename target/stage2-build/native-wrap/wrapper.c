#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "payload.h"

static int write_all(int fd, const unsigned char *buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t wrote = write(fd, buf + off, len - off);
        if (wrote <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return 0;
        }
        off += (size_t)wrote;
    }
    return 1;
}

int main(int argc, char **argv) {
    char path[] = "/tmp/vittec-native-XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) {
        fprintf(stderr, "[stage2-native][error] mkstemp failed: %s\n", strerror(errno));
        return 1;
    }
    if (!write_all(fd, vittec_payload, vittec_payload_len)) {
        fprintf(stderr, "[stage2-native][error] write payload failed: %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return 1;
    }
    if (fchmod(fd, 0700) != 0) {
        fprintf(stderr, "[stage2-native][error] chmod payload failed: %s\n", strerror(errno));
        close(fd);
        unlink(path);
        return 1;
    }
    close(fd);

    char **exec_argv = (char **)calloc((size_t)argc + 3, sizeof(char *));
    if (exec_argv == NULL) {
        fprintf(stderr, "[stage2-native][error] alloc argv failed\n");
        unlink(path);
        return 1;
    }
    exec_argv[0] = "sh";
    exec_argv[1] = path;
    for (int i = 1; i < argc; ++i) {
        exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = NULL;

    execv("/bin/sh", exec_argv);
    fprintf(stderr, "[stage2-native][error] exec /bin/sh failed: %s\n", strerror(errno));
    unlink(path);
    free(exec_argv);
    return 1;
}
