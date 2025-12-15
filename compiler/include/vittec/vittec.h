#ifndef VITTEC_INCLUDE_VITTEC_VITTEC_H
#define VITTEC_INCLUDE_VITTEC_VITTEC_H

/*
  vittec.h — public umbrella header (max)

  This header is the single include point for users embedding vittec.

  It aggregates:
  - global config and version
  - diagnostics model + emitters + source map
  - front-end token/lexer/parser (bootstrap top-level index)
  - back-end C emitter (bootstrap codegen)

  Policy:
  - Keep includes lightweight and stable.
  - Prefer forward-compatible, options-based APIs.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_PUBLIC_API_VERSION 1u

#ifdef __cplusplus
} /* extern "C" */
#endif

/* -------------------------------------------------------------------------
 * Core config / version
 * ------------------------------------------------------------------------- */

#include "vittec/config.h"
#include "vittec/version.h"

/* -------------------------------------------------------------------------
 * Diagnostics
 * ------------------------------------------------------------------------- */

#include "vittec/diag/severity.h"
#include "vittec/diag/span.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/emitter.h"

/* -------------------------------------------------------------------------
 * Front-end
 * ------------------------------------------------------------------------- */

#include "vittec/front/token.h"
#include "vittec/front/lexer.h"
#include "vittec/front/parser.h"

/* -------------------------------------------------------------------------
 * Back-end
 * ------------------------------------------------------------------------- */

#include "vittec/back/emit_c.h"

/* -------------------------------------------------------------------------
 * Notes
 * -------------------------------------------------------------------------
 *
 * Typical bootstrap usage:
 *
 *   vittec_source_map_t sm; vittec_source_map_init(&sm);
 *   vittec_file_id_t fid; vittec_source_map_add_path(&sm, "main.vitte", &fid);
 *
 *   const uint8_t* data; uint32_t len;
 *   data = vittec_source_map_file_data(&sm, fid, &len);
 *
 *   vittec_diag_sink_t sink; vittec_diag_sink_init(&sink);
 *   vittec_diag_bag_t bag; vittec_diag_bag_init(&bag);
 *
 *   vittec_lexer_t lx;
 *   vittec_lexer_options_t lopt; vittec_lexer_options_init(&lopt);
 *   vittec_lexer_init_ex(&lx, data, len, fid, &sink, &bag, &lopt);
 *
 *   vittec_parse_unit_t u; vittec_parse_unit_init(&u);
 *   vittec_parse_options_t popt; vittec_parse_options_init(&popt);
 *   vittec_parse_unit_ex(&lx, &u, &popt);
 *
 *   if(vittec_diag_bag_has_errors(&bag)) {
 *     vittec_emit_human_bag(&sm, &bag);
 *   } else {
 *     vittec_emit_c_options_t eopt; vittec_emit_c_options_init(&eopt);
 *     vittec_emit_c_file_ex(&u, "out/generated.c", &eopt);
 *   }
 *
 *   vittec_parse_unit_free(&u);
 *   vittec_diag_bag_free(&bag);
 *   vittec_diag_sink_free(&sink);
 *   vittec_source_map_free(&sm);
 *
 */

#endif /* VITTEC_INCLUDE_VITTEC_VITTEC_H */
