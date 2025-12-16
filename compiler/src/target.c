#include "compiler/target.h"
#include <stdlib.h>

target_info_t* target_info_create(target_arch_t arch, target_os_t os) {
    target_info_t *ti = (target_info_t *)malloc(sizeof(target_info_t));
    if (!ti) return NULL;
    
    ti->arch = arch;
    ti->os = os;
    
    // Default x86_64 values
    ti->pointer_size = 8;
    ti->alignment = 8;
    
    return ti;
}

void target_info_destroy(target_info_t *ti) {
    if (!ti) return;
    free(ti);
}
