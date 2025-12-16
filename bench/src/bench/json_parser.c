#include "bench/json_parser.h"
#include <string.h>

/*
  json_parser.c

  Lightweight JSON parsing utilities.
*/

bench_json_parser_t bench_json_parser_new(const char* str) {
  bench_json_parser_t p = {0};
  if (str) {
    p.pos = str;
    p.end = str + strlen(str);
  }
  return p;
}

static void skip_whitespace(bench_json_parser_t* p) {
  while (p->pos < p->end) {
    char c = *p->pos;
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      break;
    }
    p->pos++;
  }
}

static int parse_string(bench_json_parser_t* p) {
  skip_whitespace(p);
  if (p->pos >= p->end || *p->pos != '"') {
    p->error = 1;
    return 0;
  }
  p->pos++;

  while (p->pos < p->end && *p->pos != '"') {
    if (*p->pos == '\\') p->pos++;
    p->pos++;
  }

  if (p->pos >= p->end) {
    p->error = 2;
    return 0;
  }
  p->pos++;
  return 1;
}

static int parse_number(bench_json_parser_t* p) {
  skip_whitespace(p);
  if (p->pos >= p->end) {
    p->error = 3;
    return 0;
  }

  if (*p->pos == '-') p->pos++;

  if (p->pos >= p->end || (*p->pos < '0' || *p->pos > '9')) {
    p->error = 4;
    return 0;
  }

  while (p->pos < p->end && *p->pos >= '0' && *p->pos <= '9') {
    p->pos++;
  }

  if (p->pos < p->end && *p->pos == '.') {
    p->pos++;
    if (p->pos >= p->end || (*p->pos < '0' || *p->pos > '9')) {
      p->error = 5;
      return 0;
    }
    while (p->pos < p->end && *p->pos >= '0' && *p->pos <= '9') {
      p->pos++;
    }
  }

  if (p->pos < p->end && (*p->pos == 'e' || *p->pos == 'E')) {
    p->pos++;
    if (p->pos < p->end && (*p->pos == '+' || *p->pos == '-')) p->pos++;
    if (p->pos >= p->end || (*p->pos < '0' || *p->pos > '9')) {
      p->error = 6;
      return 0;
    }
    while (p->pos < p->end && *p->pos >= '0' && *p->pos <= '9') {
      p->pos++;
    }
  }

  return 1;
}

int bench_json_parse_array(bench_json_parser_t* p) {
  skip_whitespace(p);
  if (p->pos >= p->end || *p->pos != '[') {
    p->error = 7;
    return 0;
  }
  p->pos++;

  skip_whitespace(p);
  if (p->pos < p->end && *p->pos == ']') {
    p->pos++;
    return 1;
  }

  while (p->pos < p->end) {
    if (!bench_json_parse_value(p)) return 0;

    skip_whitespace(p);
    if (p->pos >= p->end) {
      p->error = 8;
      return 0;
    }

    if (*p->pos == ',') {
      p->pos++;
      skip_whitespace(p);
    } else if (*p->pos == ']') {
      p->pos++;
      return 1;
    } else {
      p->error = 9;
      return 0;
    }
  }

  p->error = 10;
  return 0;
}

int bench_json_parse_object(bench_json_parser_t* p) {
  skip_whitespace(p);
  if (p->pos >= p->end || *p->pos != '{') {
    p->error = 11;
    return 0;
  }
  p->pos++;

  skip_whitespace(p);
  if (p->pos < p->end && *p->pos == '}') {
    p->pos++;
    return 1;
  }

  while (p->pos < p->end) {
    if (!parse_string(p)) return 0;

    skip_whitespace(p);
    if (p->pos >= p->end || *p->pos != ':') {
      p->error = 12;
      return 0;
    }
    p->pos++;

    if (!bench_json_parse_value(p)) return 0;

    skip_whitespace(p);
    if (p->pos >= p->end) {
      p->error = 13;
      return 0;
    }

    if (*p->pos == ',') {
      p->pos++;
    } else if (*p->pos == '}') {
      p->pos++;
      return 1;
    } else {
      p->error = 14;
      return 0;
    }
  }

  p->error = 15;
  return 0;
}

int bench_json_parse_value(bench_json_parser_t* p) {
  skip_whitespace(p);

  if (p->pos >= p->end) {
    p->error = 16;
    return 0;
  }

  if (*p->pos == '{') {
    return bench_json_parse_object(p);
  } else if (*p->pos == '[') {
    return bench_json_parse_array(p);
  } else if (*p->pos == '"') {
    return parse_string(p);
  } else if ((*p->pos >= '0' && *p->pos <= '9') || *p->pos == '-') {
    return parse_number(p);
  } else if (p->pos + 4 <= p->end && strncmp(p->pos, "true", 4) == 0) {
    p->pos += 4;
    return 1;
  } else if (p->pos + 5 <= p->end && strncmp(p->pos, "false", 5) == 0) {
    p->pos += 5;
    return 1;
  } else if (p->pos + 4 <= p->end && strncmp(p->pos, "null", 4) == 0) {
    p->pos += 4;
    return 1;
  }

  p->error = 17;
  return 0;
}

int bench_json_parser_error(const bench_json_parser_t* parser) {
  return parser ? parser->error : -1;
}

size_t bench_json_parser_pos(const bench_json_parser_t* parser) {
  if (!parser) return 0;
  return parser->pos - (parser->end - (parser->pos > parser->end ? 0 : strlen(parser->end)));
}
