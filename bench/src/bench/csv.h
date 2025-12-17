// csv.h — tiny CSV writer/parser helpers (C17)

#ifndef VITTE_BENCH_CSV_H
#define VITTE_BENCH_CSV_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Write one CSV field with correct escaping.
// Returns 1 on success, 0 on write error.
int csv_write_field(FILE* out, const char* s);

// Write a separator (comma) if not first column.
// `*inout_first` should be 1 for first column; it will be set to 0.
int csv_write_sep(FILE* out, int* inout_first);

// Write end-of-record (\n).
int csv_write_eol(FILE* out);

// Convenience: write a whole record from an array of C strings.
int csv_write_record(FILE* out, const char* const* fields, size_t nfields);

// -----------------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------------

typedef struct csv_fields {
  char**  v;    // field pointers (into buf)
  size_t  n;    // number of fields
  char*   buf;  // backing storage
} csv_fields;

// Parse a CSV line into fields.
// Allocates memory for `out->v` and `out->buf`; free with csv_fields_free().
// Returns 1 on success, 0 on parse/allocation failure.
int  csv_parse_line(const char* line, csv_fields* out);

// Free resources allocated by csv_parse_line().
void csv_fields_free(csv_fields* f);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_CSV_H
