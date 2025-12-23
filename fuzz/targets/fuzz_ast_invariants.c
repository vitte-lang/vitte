// fuzz_ast_invariants.c
// Fuzz target: phrase AST structural invariants after parsing.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz/fuzz.h"

#include "vitte/diag.h"
#include "vitte/parser_phrase.h"
#include "vitte/vitte.h"

static int list_has_cycle(const vitte_ast* head) {
  const vitte_ast* slow = head;
  const vitte_ast* fast = head;
  while (fast && fast->next) {
    slow = slow->next;
    fast = fast->next->next;
    if (slow == fast)
      return 1;
  }
  return 0;
}

static const vitte_ast* list_last_bounded(const vitte_ast* head, size_t limit, size_t* out_count) {
  size_t n = 0;
  const vitte_ast* it = head;
  const vitte_ast* last = NULL;
  while (it && n < limit) {
    last = it;
    it = it->next;
    n++;
  }
  if (out_count)
    *out_count = n;
  return last;
}

static void check_node_header(const vitte_ast* node) {
  FUZZ_ASSERT(node);
  FUZZ_ASSERT(node->kind >= VITTE_AST_PHR_UNIT);
  FUZZ_ASSERT(node->kind <= VITTE_AST_EXPR_PATH);
  FUZZ_ASSERT(node->span.hi >= node->span.lo);
}

FUZZ_TARGET(fuzz_ast_invariants_target) {
  size_t cap = size;
  if (cap > (1u << 20))
    cap = (1u << 20);

  char* src = (char*)malloc(cap + 1);
  if (!src)
    return 0;
  if (cap)
    memcpy(src, data, cap);
  src[cap] = '\0';

  vitte_ctx ctx;
  vitte_ctx_init(&ctx);
  vitte_diag_bag diags;
  vitte_diag_bag_init(&diags);

  vitte_ast* ast = NULL;
  vitte_result pr = vitte_parse_phrase(&ctx, 0u, src, cap, &ast, &diags);
  if (pr == VITTE_OK && ast) {
    // Basic acyclicity at root list (shouldn't have next siblings, but check anyway).
    FUZZ_ASSERT(!list_has_cycle(ast));

    // Walk tree iteratively with a small bounded stack.
    const size_t kMaxNodes = 200000;
    const size_t kMaxListWalk = 200000;

    size_t stack_cap = 256;
    size_t sp = 0;
    const vitte_ast** stack = (const vitte_ast**)malloc(stack_cap * sizeof(*stack));
    if (!stack)
      goto out;
    stack[sp++] = ast;

    size_t visited = 0;
    while (sp) {
      const vitte_ast* node = stack[--sp];
      check_node_header(node);
      visited++;
      if (visited > kMaxNodes)
        break;

      const vitte_ast* first = node->first_child;
      const vitte_ast* last = node->last_child;
      if (!first) {
        FUZZ_ASSERT(last == NULL);
        continue;
      }
      FUZZ_ASSERT(last != NULL);
      FUZZ_ASSERT(!list_has_cycle(first));

      size_t child_count = 0;
      const vitte_ast* computed_last = list_last_bounded(first, kMaxListWalk, &child_count);
      FUZZ_ASSERT(computed_last != NULL);
      FUZZ_ASSERT(computed_last == last);

      // Span nesting (best-effort): children should be within parent range.
      const vitte_ast* it = first;
      while (it) {
        FUZZ_ASSERT(it->span.lo >= node->span.lo);
        FUZZ_ASSERT(it->span.hi <= node->span.hi);
        it = it->next;
      }

      // Push children.
      it = first;
      while (it) {
        if (sp == stack_cap) {
          size_t new_cap = stack_cap * 2;
          const vitte_ast** ns = (const vitte_ast**)realloc((void*)stack, new_cap * sizeof(*stack));
          if (!ns) {
            free((void*)stack);
            goto out;
          }
          stack = ns;
          stack_cap = new_cap;
        }
        stack[sp++] = it;
        it = it->next;
      }
    }
    free((void*)stack);
  }

out:
  if (ast)
    vitte_ast_free(&ctx, ast);
  vitte_diag_bag_free(&diags);
  vitte_ctx_free(&ctx);
  free(src);
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_ast_invariants_target
#include "fuzz/fuzz_driver.h"
