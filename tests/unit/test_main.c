#include <stdio.h>

/* Declare tests from other files */
void test_lexer_basic(void);
void test_vm_create(void);
void test_asm_fastpaths_verify(void);

int main(void) {
    test_lexer_basic();
    test_vm_create();
    test_asm_fastpaths_verify();
    printf("All unit tests passed.\n");
    return 0;
}
