#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "compiler/symbol_table.h"

#define TEST(name) printf("TEST: %s\n", name)
#define ASSERT(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))

void test_symbol_table_create(void) {
    TEST("symbol_table_create");
    
    symbol_table_t *st = symbol_table_create();
    ASSERT(st != NULL);
    ASSERT_EQ(st->count, 0);
    ASSERT_EQ(st->scope_level, 0);
    ASSERT(st->symbols != NULL);
    
    symbol_table_destroy(st);
}

void test_symbol_table_scope(void) {
    TEST("symbol_table_scope");
    
    symbol_table_t *st = symbol_table_create();
    ASSERT(st != NULL);
    ASSERT_EQ(st->scope_level, 0);
    
    symbol_table_enter_scope(st);
    ASSERT_EQ(st->scope_level, 1);
    
    symbol_table_enter_scope(st);
    ASSERT_EQ(st->scope_level, 2);
    
    symbol_table_exit_scope(st);
    ASSERT_EQ(st->scope_level, 1);
    
    symbol_table_exit_scope(st);
    ASSERT_EQ(st->scope_level, 0);
    
    symbol_table_destroy(st);
}

int main(void) {
    printf("=== Symbol Table Unit Tests ===\n\n");
    
    test_symbol_table_create();
    test_symbol_table_scope();
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}
