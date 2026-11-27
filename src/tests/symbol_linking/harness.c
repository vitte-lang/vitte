#include <stdint.h>

#ifdef VITTE_NATIVE_FRONTEND
#define SYMBOL_LINK_A tests_symbol_linking_module_a_symbol_link_a
#define SYMBOL_LINK_B tests_symbol_linking_module_b_symbol_link_b
#else
#define SYMBOL_LINK_A symbol_link_a
#define SYMBOL_LINK_B symbol_link_b
#endif

int64_t SYMBOL_LINK_A(void);
int64_t SYMBOL_LINK_B(void);

int main(void) {
    return SYMBOL_LINK_A() + SYMBOL_LINK_B() == INT64_C(42) ? 0 : 1;
}
