#ifndef VITTE_SRC_DIAG_CODES_H
#define VITTE_SRC_DIAG_CODES_H

#include "vitte/vitte.h"

static inline const char* vitte_errc_code(vitte_error_code c) {
    switch (c) {
        case VITTE_ERRC_UNEXPECTED_EOF: return "V0001";
        case VITTE_ERRC_UNMATCHED_END: return "V0002";
        case VITTE_ERRC_MISSING_END: return "V0003";
        case VITTE_ERRC_SYNTAX: return "V0004";
        case VITTE_ERRC_NONE: return "V0000";
        default: return "V0000";
    }
}

static inline const char* vitte_errc_help(vitte_error_code c) {
    switch (c) {
        case VITTE_ERRC_UNEXPECTED_EOF: return "file ended unexpectedly; check for missing blocks/terminators";
        case VITTE_ERRC_UNMATCHED_END: return "remove the extra `end`, or add the matching opener";
        case VITTE_ERRC_MISSING_END: return "add a matching `end` for the open block";
        case VITTE_ERRC_SYNTAX: return "check the syntax near this location";
        default: return "";
    }
}

#endif /* VITTE_SRC_DIAG_CODES_H */

