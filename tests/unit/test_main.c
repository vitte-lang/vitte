#include <stdio.h>

/* Declare tests from other files */
void test_lexer_basic(void);
void test_vm_create(void);

int main(void) {
    test_lexer_basic();
    test_vm_create();
    printf("All unit tests passed.\n");
    return 0;
}
