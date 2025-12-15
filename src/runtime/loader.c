#include "../../include/runtime/loader.h"
#include <stdlib.h>
#include <stdio.h>

BytecodeLoader *loader_create(void) {
    BytecodeLoader *loader = malloc(sizeof(BytecodeLoader));
    loader->bytecode = NULL;
    loader->size = 0;
    return loader;
}

int loader_load_file(BytecodeLoader *loader, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    loader->size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    loader->bytecode = malloc(loader->size);
    fread(loader->bytecode, loader->size, 1, f);
    fclose(f);
    return 0;
}

void loader_destroy(BytecodeLoader *loader) {
    free(loader->bytecode);
    free(loader);
}
