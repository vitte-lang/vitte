#ifndef STEEL_PROFILER_H
#define STEEL_PROFILER_H

typedef struct {
    double start_time;
    double end_time;
} Profiler;

Profiler *profiler_create(void);
void profiler_start(Profiler *prof);
void profiler_end(Profiler *prof);
void profiler_destroy(Profiler *prof);

#endif
