#ifndef VITTE_PARSER_PHRASE_H
#define VITTE_PARSER_PHRASE_H

#include "vitte.h"

#ifdef __cplusplus
extern "C" {
#endif

vitte_result vitte_parse_phrase(vitte_ctx* ctx,
                                const char* src,
                                size_t len,
                                vitte_ast** out_ast,
                                vitte_error* err);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_PARSER_PHRASE_H */
