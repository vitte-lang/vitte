#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "compiler/ast.h"

#define TEST(name) printf("TEST: %s\n", name)
#define ASSERT(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))

void test_ast_module_create(void) {
    TEST("ast_module_create");
    
    ast_module_t *mod = ast_module_create();
    ASSERT(mod != NULL);
    ASSERT_EQ(mod->decl_count, 0);
    ASSERT(mod->decls != NULL);
    
    ast_module_destroy(mod);
}

void test_ast_module_add_decl(void) {
    TEST("ast_module_add_decl");
    
    ast_module_t *mod = ast_module_create();
    ASSERT(mod != NULL);
    
    ast_node_t node;
    node.kind = AST_NODE_VAR;
    node.line = 1;
    node.col = 1;
    node.data = NULL;
    
    ast_module_add_decl(mod, &node);
    ASSERT_EQ(mod->decl_count, 1);
    ASSERT_EQ(mod->decls[0]->kind, AST_NODE_VAR);
    
    ast_module_destroy(mod);
}

void test_ast_module_capacity_growth(void) {
    TEST("ast_module_capacity_growth");
    
    ast_module_t *mod = ast_module_create();
    ASSERT(mod != NULL);
    
    ast_node_t node;
    node.kind = AST_NODE_VAR;
    node.line = 1;
    node.col = 1;
    node.data = NULL;
    
    for (int i = 0; i < 100; i++) {
        ast_module_add_decl(mod, &node);
    }
    
    ASSERT_EQ(mod->decl_count, 100);
    
    ast_module_destroy(mod);
}

int main(void) {
    printf("=== AST Unit Tests ===\n\n");
    
    test_ast_module_create();
    test_ast_module_add_decl();
    test_ast_module_capacity_growth();
    
    printf("\n=== All tests passed! ===\n");
    
    return 0;
}
