#ifndef VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H
    #define VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H



    #include "vittec/diag/diagnostic.h"
#include "vittec/diag/source_map.h"

void vittec_emit_human(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags);
void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags);

    #endif /* VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H */
