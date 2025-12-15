#include "../../include/debug/profiler.h"
#include <stdlib.h>

Profiler *profiler_create(void) {
    Profiler *prof = malloc(sizeof(Profiler));
    prof->start_time = 0;
    prof->end_time = 0;
    return prof;
}

void profiler_start(Profiler *prof) {
    // Record start time
}

void profiler_end(Profiler *prof) {
    // Record end time
}

void profiler_destroy(Profiler *prof) {
    free(prof);
}
