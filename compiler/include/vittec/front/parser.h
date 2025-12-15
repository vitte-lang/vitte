#ifndef VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H
    #define VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H



    #include "vittec/front/lexer.h"

typedef struct vittec_parse_unit {
  int has_main;
} vittec_parse_unit_t;

/* Parser minimal: détecte juste `fn main` et saute les blocs `.end`. */
int vittec_parse_unit(vittec_lexer_t* lx, vittec_parse_unit_t* out);

    #endif /* VITTEC_INCLUDE_VITTEC_FRONT_PARSER_H */
