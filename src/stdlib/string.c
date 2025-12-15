#include "../../include/stdlib/string.h"
#include <string.h>
#include <stdlib.h>

int steel_strlen(const char *str) {
    return strlen(str);
}

char *steel_strcat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
