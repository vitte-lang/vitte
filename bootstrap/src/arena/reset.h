#ifndef VITTE_BOOTSTRAP_ARENA_RESET_H
#define VITTE_BOOTSTRAP_ARENA_RESET_H

#include "arena.h"

#ifdef __cplusplus
extern "C" {
#endif

void vitte_arena_reset_keep_first(vitte_arena_t *arena);
void vitte_arena_reset_release_all(vitte_arena_t *arena);
void vitte_arena_reset_with_policy(vitte_arena_t *arena, vitte_arena_reset_policy_t policy);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_RESET_H */
