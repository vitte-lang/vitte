#ifndef VITTE_PARSER_PHRASE_H
#define VITTE_PARSER_PHRASE_H

#include "vitte.h"
#include "vitte/diag.h"

#ifdef __cplusplus
extern "C" {
#endif

vitte_result vitte_parse_phrase(vitte_ctx* ctx,
                                vitte_file_id file_id,
                                const char* src,
                                size_t len,
                                vitte_ast** out_ast,
                                vitte_diag_bag* diags);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_PARSER_PHRASE_H */
