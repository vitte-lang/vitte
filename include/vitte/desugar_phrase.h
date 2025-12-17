#ifndef VITTE_DESUGAR_PHRASE_H
#define VITTE_DESUGAR_PHRASE_H

#include "vitte.h"

#ifdef __cplusplus
extern "C" {
#endif

vitte_result vitte_desugar_phrase(vitte_ctx* ctx,
                                  vitte_ast* phrase_ast,
                                  vitte_ast** out_core_ast,
                                  vitte_error* err);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DESUGAR_PHRASE_H */
