#ifndef STEEL_LOADER_H
#define STEEL_LOADER_H

#include <stddef.h>

typedef struct {
    unsigned char *bytecode;
    size_t size;
} BytecodeLoader;

BytecodeLoader *loader_create(void);
int loader_load_file(BytecodeLoader *loader, const char *filename);
void loader_destroy(BytecodeLoader *loader);

#endif
