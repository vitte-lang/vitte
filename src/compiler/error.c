#include "../../include/compiler/error.h"
#include <stdio.h>
#include <stdlib.h>

void error_report(const char *filename, int line, int column, const char *message) {
    fprintf(stderr, "%s:%d:%d: error: %s\n", filename, line, column, message);
}

void error_warning(const char *filename, int line, int column, const char *message) {
    fprintf(stderr, "%s:%d:%d: warning: %s\n", filename, line, column, message);
}
