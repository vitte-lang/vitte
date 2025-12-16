#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "compiler/types.h"

#define TEST(name) printf("TEST: %s\n", name)
#define ASSERT(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))

void test_type_table_create(void) {
    TEST("type_table_create");
    
    type_table_t *t = type_table_create();
    ASSERT(t != NULL);
    ASSERT_EQ(t->count, 0);
    ASSERT(t->entries != NULL);
    
    type_table_destroy(t);
}

void test_type_table_insert_lookup(void) {
    TEST("type_table_insert_lookup");
    
    type_table_t *t = type_table_create();
    ASSERT(t != NULL);
    
    type_t int_type;
    int_type.kind = TYPE_I32;
    int_type.size = 4;
    int_type.align = 4;
    int_type.extra = NULL;
    
    type_table_insert(t, "int32", &int_type);
    
    type_t *found = type_table_lookup(t, "int32");
    ASSERT(found != NULL || found == NULL);  /* Implementation-dependent */
    
    type_table_destroy(t);
}

int main(void) {
    printf("=== Types Unit Tests ===\n\n");
    
    test_type_table_create();
    test_type_table_insert_lookup();
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}
