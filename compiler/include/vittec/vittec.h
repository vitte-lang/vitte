#ifndef VITTEC_INCLUDE_VITTEC_VITTEC_H
    #define VITTEC_INCLUDE_VITTEC_VITTEC_H

    #include <stdint.h>

    typedef struct vittec_session vittec_session_t;

typedef enum vittec_emit_kind {
  VITTEC_EMIT_TOKENS = 1,
  VITTEC_EMIT_C = 2
} vittec_emit_kind_t;

typedef struct vittec_compile_options {
  const char* input_path;
  const char* output_path;     /* optional */
  vittec_emit_kind_t emit_kind;
  int json_diagnostics;        /* 0/1 */
} vittec_compile_options_t;

vittec_session_t* vittec_session_new(void);
void vittec_session_free(vittec_session_t* s);

int vittec_compile(vittec_session_t* s, const vittec_compile_options_t* opt);

    #endif /* VITTEC_INCLUDE_VITTEC_VITTEC_H */
