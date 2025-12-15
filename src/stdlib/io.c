#include "../../include/stdlib/io.h"
#include <stdio.h>
#include <stdlib.h>

void steel_print(const char *str) {
    printf("%s\n", str);
}

void steel_println(const char *str) {
    printf("%s\n", str);
}

char *steel_input(void) {
    char buffer[256];
    fgets(buffer, sizeof(buffer), stdin);
    return buffer;
}
