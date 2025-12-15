#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("Steel Compiler v0.1.0\n");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    // TODO: Compiler implementation
    
    return 0;
}
