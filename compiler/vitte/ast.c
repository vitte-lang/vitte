
#include "vitte/vitte.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

//------------------------------------------------------------------------------
// Error (thread-local)
//------------------------------------------------------------------------------

static _Thread_local char g_vitte_ast_err_[256];

static void vitte_set_err_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_vitte_ast_err_)) n = sizeof(g_vitte_ast_err_) - 1;
    memcpy(g_vitte_ast_err_, msg, n);
    g_vitte_ast_err_[n] = 0;
}

// Public getter (safe even if not declared yet in headers)
const char* vitte_ast_last_error(void)
{
    return g_vitte_ast_err_;
}

//------------------------------------------------------------------------------
// Small helpers
//------------------------------------------------------------------------------

static char* vitte_strdup_n_(const char* s, size_t n)
{
    if (!s) return NULL;
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static char* vitte_strdup_(const char* s)
{
    if (!s) s = "";
    return vitte_strdup_n_(s, strlen(s));
}

static void vitte_ast_zero_(vitte_ast* n)
{
    // Only touch fields we know exist.
    n->next = NULL;
    n->first_child = NULL;
    n->text = NULL;
    n->aux_text = NULL;
}

//------------------------------------------------------------------------------
// Context
//------------------------------------------------------------------------------

void vitte_ctx_init(vitte_ctx* ctx)
{
    if (ctx)
    {
        // Keep existing semantics.
        ctx->reserved = 0;
    }
}

void vitte_ctx_free(vitte_ctx* ctx)
{
    // No owned resources at the moment.
    (void)ctx;
}

//------------------------------------------------------------------------------
// AST constructors / mutators (malloc-based)
//------------------------------------------------------------------------------

// Allocate a new node, zeroed for the known fields.
// If your headers already declare this, it will satisfy the linker.
vitte_ast* vitte_ast_new(vitte_ctx* ctx)
{
    (void)ctx;

    vitte_ast* n = (vitte_ast*)malloc(sizeof(vitte_ast));
    if (!n)
    {
        vitte_set_err_("vitte_ast_new: out of memory");
        return NULL;
    }

    vitte_ast_zero_(n);
    return n;
}

// Replace node->text with a duplicated string.
bool vitte_ast_set_text(vitte_ast* n, const char* s)
{
    if (!n)
    {
        vitte_set_err_("vitte_ast_set_text: null node");
        return false;
    }

    char* copy = vitte_strdup_(s);
    if (!copy)
    {
        vitte_set_err_("vitte_ast_set_text: out of memory");
        return false;
    }

    free(n->text);
    n->text = copy;
    return true;
}

// Replace node->aux_text with a duplicated string.
bool vitte_ast_set_aux_text(vitte_ast* n, const char* s)
{
    if (!n)
    {
        vitte_set_err_("vitte_ast_set_aux_text: null node");
        return false;
    }

    char* copy = vitte_strdup_(s);
    if (!copy)
    {
        vitte_set_err_("vitte_ast_set_aux_text: out of memory");
        return false;
    }

    free(n->aux_text);
    n->aux_text = copy;
    return true;
}

// Detach a node from any sibling list (does not touch children).
void vitte_ast_detach(vitte_ast* n)
{
    if (!n) return;
    n->next = NULL;
}

// Append `child` as last child of `parent`.
// Returns the appended child (for chaining) or NULL on error.
vitte_ast* vitte_ast_append_child(vitte_ast* parent, vitte_ast* child)
{
    if (!parent || !child)
    {
        vitte_set_err_("vitte_ast_append_child: null arg");
        return NULL;
    }

    // Ensure child is not already linked to an arbitrary list.
    // We don't attempt to preserve existing siblings.
    child->next = NULL;

    if (!parent->first_child)
    {
        parent->first_child = child;
        return child;
    }

    vitte_ast* it = parent->first_child;
    while (it->next)
        it = it->next;

    it->next = child;
    return child;
}

// Append `sib` at end of sibling chain starting at `n`.
vitte_ast* vitte_ast_append_sibling(vitte_ast* n, vitte_ast* sib)
{
    if (!n || !sib)
    {
        vitte_set_err_("vitte_ast_append_sibling: null arg");
        return NULL;
    }

    sib->next = NULL;

    vitte_ast* it = n;
    while (it->next)
        it = it->next;

    it->next = sib;
    return sib;
}

// Count direct children.
size_t vitte_ast_child_count(const vitte_ast* n)
{
    if (!n) return 0;
    size_t c = 0;
    for (const vitte_ast* it = n->first_child; it; it = it->next)
        c++;
    return c;
}

// Best-effort total node count for a forest/list.
size_t vitte_ast_count_nodes(const vitte_ast* root)
{
    size_t count = 0;

    // Manual stack of pointers to avoid recursion.
    const vitte_ast** stack = NULL;
    size_t sp = 0, cap = 0;

    const vitte_ast* cur = root;

    while (cur || sp)
    {
        if (!cur)
        {
            cur = stack[--sp];
            continue;
        }

        count++;

        // Push next sibling then go down into child first.
        if (cur->next)
        {
            if (sp == cap)
            {
                size_t ncap = cap ? cap * 2 : 64;
                const vitte_ast** ns = (const vitte_ast**)realloc((void*)stack, ncap * sizeof(*stack));
                if (!ns)
                {
                    // OOM: stop counting deterministically.
                    break;
                }
                stack = ns;
                cap = ncap;
            }
            stack[sp++] = cur->next;
        }

        cur = cur->first_child;
    }

    free((void*)stack);
    return count;
}

//------------------------------------------------------------------------------
// Free (iterative, stack-based)
//------------------------------------------------------------------------------

static void vitte_ast_free_list_iter_(vitte_ast* node)
{
    if (!node) return;

    // We perform a DFS using an explicit stack, freeing nodes immediately
    // after pushing their children/siblings.

    vitte_ast** stack = NULL;
    size_t sp = 0, cap = 0;

    vitte_ast* cur = node;

    while (cur || sp)
    {
        if (!cur)
        {
            cur = stack[--sp];
            continue;
        }

        // Save pointers before freeing.
        vitte_ast* next = cur->next;
        vitte_ast* child = cur->first_child;

        // Push next sibling first (so children get processed next).
        if (next)
        {
            if (sp == cap)
            {
                size_t ncap = cap ? cap * 2 : 64;
                vitte_ast** ns = (vitte_ast**)realloc(stack, ncap * sizeof(*stack));
                if (!ns)
                {
                    // OOM during free: degrade gracefully by switching to
                    // iterative sibling-walk without stack expansion.
                    // We'll free current and then continue with `next`.
                    // Children will still be freed by walking child list.
                    // This is still safe, just less optimal.
                    free(cur->text);
                    free(cur->aux_text);
                    free(cur);
                    cur = child;
                    // If child chain is deep and stack couldn't grow, we might
                    // not visit `next` later; so fallback by freeing `next` as
                    // a sibling list after finishing `child`.
                    // Best-effort: free `next` immediately as well.
                    vitte_ast_free_list_iter_(next);
                    continue;
                }
                stack = ns;
                cap = ncap;
            }
            stack[sp++] = next;
        }

        // Free current.
        free(cur->text);
        free(cur->aux_text);
        free(cur);

        // Continue with child.
        cur = child;
    }

    free(stack);
}

void vitte_ast_free(vitte_ctx* ctx, vitte_ast* ast)
{
    (void)ctx;
    vitte_ast_free_list_iter_(ast);
}

