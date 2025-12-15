#include "../../include/stdlib/time.h"
#include <time.h>

double steel_time(void) {
    return (double)time(NULL);
}

double steel_clock(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

void steel_sleep(double seconds) {
    // Platform-specific sleep
}
