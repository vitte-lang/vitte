#include <stdint.h>

int64_t symbol_link_a(void);
int64_t symbol_link_b(void);

int main(void) {
    return symbol_link_a() + symbol_link_b() == INT64_C(42) ? 0 : 1;
}
