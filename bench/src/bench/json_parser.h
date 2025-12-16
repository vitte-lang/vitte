#pragma once

/*
  json_parser.h

  Lightweight JSON parsing utilities.
*/

#ifndef VITTE_BENCH_JSON_PARSER_H
#define VITTE_BENCH_JSON_PARSER_H

#include <stddef.h>

typedef struct {
  const char* pos;
  const char* end;
  int error;
} bench_json_parser_t;

/* Create parser from string */
bench_json_parser_t bench_json_parser_new(const char* str);

/* Parse a complete JSON value */
int bench_json_parse_value(bench_json_parser_t* parser);

/* Parse JSON object */
int bench_json_parse_object(bench_json_parser_t* parser);

/* Parse JSON array */
int bench_json_parse_array(bench_json_parser_t* parser);

/* Get parser error code */
int bench_json_parser_error(const bench_json_parser_t* parser);

/* Get current position */
size_t bench_json_parser_pos(const bench_json_parser_t* parser);

#endif /* VITTE_BENCH_JSON_PARSER_H */
