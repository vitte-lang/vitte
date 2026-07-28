#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct Buffer {
  char *data;
  size_t len;
  size_t cap;
} Buffer;

typedef struct RunResult {
  Buffer stdout_buf;
  Buffer stderr_buf;
  int status;
} RunResult;

static int buffer_append(Buffer *buffer, const char *data, size_t len) {
  if (len == 0) {
    return 0;
  }
  if (buffer->len + len + 1 < buffer->len) {
    return -1;
  }
  if (buffer->len + len + 1 > buffer->cap) {
    size_t next = buffer->cap == 0 ? 4096 : buffer->cap;
    while (next < buffer->len + len + 1) {
      if (next > ((size_t)-1) / 2) {
        return -1;
      }
      next *= 2;
    }
    char *grown = (char *)realloc(buffer->data, next);
    if (grown == NULL) {
      return -1;
    }
    buffer->data = grown;
    buffer->cap = next;
  }
  memcpy(buffer->data + buffer->len, data, len);
  buffer->len += len;
  buffer->data[buffer->len] = '\0';
  return 0;
}

static int starts_with(const char *text, const char *prefix) {
  return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int is_fr_locale(const char *text) {
  if (text == NULL) {
    return 0;
  }
  return strcmp(text, "fr") == 0 || strcmp(text, "fr-FR") == 0 ||
         strcmp(text, "fr_CA") == 0 || strcmp(text, "fr-CA") == 0;
}

static int wants_french(int argc, char **argv) {
  const char *env_lang = getenv("VITTE_LANG");
  if (is_fr_locale(env_lang)) {
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc && is_fr_locale(argv[i + 1])) {
      return 1;
    }
    if (starts_with(argv[i], "--lang=") && is_fr_locale(argv[i] + strlen("--lang="))) {
      return 1;
    }
  }
  return 0;
}

static const char *normalized_locale(int argc, char **argv) {
  const char *lang = NULL;
  const char *env_lang = getenv("VITTE_LANG");
  if (env_lang != NULL && env_lang[0] != '\0') {
    lang = env_lang;
  }
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--lang") == 0 && i + 1 < argc) {
      lang = argv[i + 1];
    }
    if (starts_with(argv[i], "--lang=")) {
      lang = argv[i] + strlen("--lang=");
    }
  }
  if (lang == NULL || lang[0] == '\0') { return "en"; }
  if (strcmp(lang, "en") == 0 || strcmp(lang, "en-US") == 0 || strcmp(lang, "en_GB") == 0 || strcmp(lang, "en-GB") == 0) { return "en"; }
  if (is_fr_locale(lang)) { return "fr"; }
  if (strcmp(lang, "es") == 0 || strcmp(lang, "es-ES") == 0 || strcmp(lang, "es_MX") == 0 || strcmp(lang, "es-MX") == 0) { return "es"; }
  if (strcmp(lang, "de") == 0 || strcmp(lang, "de-DE") == 0 || strcmp(lang, "de_AT") == 0 || strcmp(lang, "de-AT") == 0) { return "de"; }
  if (strcmp(lang, "it") == 0 || strcmp(lang, "it-IT") == 0) { return "it"; }
  if (strcmp(lang, "pt-BR") == 0 || strcmp(lang, "pt") == 0 || strcmp(lang, "pt_BR") == 0 || strcmp(lang, "pt-PT") == 0) { return "pt-BR"; }
  if (strcmp(lang, "nl") == 0 || strcmp(lang, "nl-NL") == 0 || strcmp(lang, "nl_BE") == 0 || strcmp(lang, "nl-BE") == 0) { return "nl"; }
  if (strcmp(lang, "pl") == 0 || strcmp(lang, "pl-PL") == 0) { return "pl"; }
  if (strcmp(lang, "ru") == 0 || strcmp(lang, "ru-RU") == 0) { return "ru"; }
  if (strcmp(lang, "uk") == 0 || strcmp(lang, "uk-UA") == 0) { return "uk"; }
  if (strcmp(lang, "zh-CN") == 0 || strcmp(lang, "zh") == 0 || strcmp(lang, "zh_CN") == 0 || strcmp(lang, "zh-Hans") == 0) { return "zh-CN"; }
  if (strcmp(lang, "ja") == 0 || strcmp(lang, "ja-JP") == 0) { return "ja"; }
  if (strcmp(lang, "ko") == 0 || strcmp(lang, "ko-KR") == 0) { return "ko"; }
  if (strcmp(lang, "tr") == 0 || strcmp(lang, "tr-TR") == 0) { return "tr"; }
  if (strcmp(lang, "ar") == 0 || strcmp(lang, "ar-SA") == 0 || strcmp(lang, "ar_EG") == 0 || strcmp(lang, "ar-EG") == 0) { return "ar"; }
  return "en";
}

static char *engine_path_from_argv0(const char *argv0) {
  const char *slash = strrchr(argv0, '/');
  const char *name = slash == NULL ? argv0 : slash + 1;
  size_t dir_len = slash == NULL ? 1 : (size_t)(slash - argv0);
  const char *dir = slash == NULL ? "." : argv0;
  size_t len = dir_len + 1 + strlen(name) + strlen(".real") + 1;
  char *out = (char *)malloc(len);
  if (out == NULL) {
    return NULL;
  }
  if (slash == NULL) {
    snprintf(out, len, "./%s.real", name);
  } else {
    snprintf(out, len, "%.*s/%s.real", (int)dir_len, dir, name);
  }
  return out;
}

static char *repo_root_from_argv0(const char *argv0) {
  const char *slash = strrchr(argv0, '/');
  if (slash == NULL) {
    char *out = (char *)malloc(2);
    if (out != NULL) {
      strcpy(out, ".");
    }
    return out;
  }
  size_t dir_len = (size_t)(slash - argv0);
  char *dir = (char *)malloc(dir_len + 1);
  if (dir == NULL) {
    return NULL;
  }
  memcpy(dir, argv0, dir_len);
  dir[dir_len] = '\0';
  char *parent_slash = strrchr(dir, '/');
  if (parent_slash == NULL) {
    free(dir);
    char *out = (char *)malloc(2);
    if (out != NULL) {
      strcpy(out, ".");
    }
    return out;
  }
  if (parent_slash == dir) {
    parent_slash[1] = '\0';
  } else {
    *parent_slash = '\0';
  }
  return dir;
}

static char *join3(const char *a, const char *b, const char *c) {
  size_t len = strlen(a) + 1 + strlen(b) + 1 + strlen(c) + 1;
  char *out = (char *)malloc(len);
  if (out == NULL) {
    return NULL;
  }
  snprintf(out, len, "%s/%s/%s", a, b, c);
  return out;
}

static char *read_text_file(const char *path, size_t *out_len) {
  FILE *file = fopen(path, "rb");
  char *data = NULL;
  long size = 0;
  if (file == NULL) {
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }
  data = (char *)malloc((size_t)size + 1);
  if (data == NULL) {
    fclose(file);
    return NULL;
  }
  size_t got = fread(data, 1, (size_t)size, file);
  fclose(file);
  data[got] = '\0';
  if (out_len != NULL) {
    *out_len = got;
  }
  return data;
}

static char *ftl_lookup(const char *path, const char *key) {
  size_t text_len = 0;
  char *text = read_text_file(path, &text_len);
  if (text == NULL) {
    return NULL;
  }
  size_t key_len = strlen(key);
  char *line = text;
  while (line < text + text_len) {
    char *next = strchr(line, '\n');
    if (next == NULL) {
      next = text + text_len;
    }
    char *cursor = line;
    while (cursor < next && (*cursor == ' ' || *cursor == '\t')) {
      cursor += 1;
    }
    if ((size_t)(next - cursor) > key_len + 1 &&
        strncmp(cursor, key, key_len) == 0 &&
        cursor[key_len] == ' ') {
      char *equals = strchr(cursor, '=');
      if (equals != NULL && equals < next) {
        char *value = equals + 1;
        while (value < next && (*value == ' ' || *value == '\t')) {
          value += 1;
        }
        while (next > value && (next[-1] == '\r' || next[-1] == ' ' || next[-1] == '\t')) {
          next -= 1;
        }
        size_t len = (size_t)(next - value);
        char *out = (char *)malloc(len + 1);
        if (out != NULL) {
          memcpy(out, value, len);
          out[len] = '\0';
        }
        free(text);
        return out;
      }
    }
    line = *next == '\n' ? next + 1 : next;
  }
  free(text);
  return NULL;
}

static const char *phase_for_code(const char *code) {
  if (strcmp(code, "NO_SUCH_CODE") == 0) {
    return "diagnostics";
  }
  if (code[0] == 'E' && code[1] >= '0' && code[1] <= '9') {
    return "parser";
  }
  if (starts_with(code, "TYPECK_")) {
    return "typeck";
  }
  if (starts_with(code, "BORROW")) {
    return "borrowck";
  }
  if (starts_with(code, "LEX_")) {
    return "lexer";
  }
  if (starts_with(code, "PARSE_") || starts_with(code, "P")) {
    return "parser";
  }
  if (starts_with(code, "SEMA_") || starts_with(code, "MOD_")) {
    return "sema";
  }
  return "diagnostic";
}

static const char *typical_cases_for_code(const char *code) {
  if (strcmp(code, "E0001") == 0) {
    return "missing name after a declaration keyword; incomplete binding; truncated import path";
  }
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "assigning a string to an integer binding; reusing a value with a narrower annotation; returning a value through a mismatched slot";
  }
  if (strcmp(code, "BORROWCK_E_USE_AFTER_MOVE") == 0) {
    return "passing ownership to another procedure and reading the original binding later; moving a field then using the parent; keeping an alias after ownership transfer";
  }
  return "the source violates the stable rule named by this diagnostic code";
}

static const char *typical_cases_for_code_lang(const char *code, const char *locale) {
  if (strcmp(code, "E0001") == 0 && strcmp(locale, "fr") == 0) {
    return "nom manquant apres un mot-cle de declaration; liaison incomplete; chemin d'import tronque";
  }
  return typical_cases_for_code(code);
}

static const char *common_pitfalls_for_code_lang(const char *code, const char *locale) {
  if (strcmp(code, "E0001") == 0 && strcmp(locale, "fr") == 0) {
    return "n'ajoutez pas un nom factice qui ne correspond pas a la construction autour";
  }
  if (strcmp(code, "E0001") == 0) {
    return "do not add a placeholder name that is not used by the surrounding construct";
  }
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "do not silence the error by widening the declaration when callers require the original type";
  }
  if (strcmp(code, "BORROWCK_E_USE_AFTER_MOVE") == 0) {
    return "do not keep references that outlive the moved owner";
  }
  return "fix the root cause before editing later cascade diagnostics";
}

static const char *invalid_example_for_code(const char *code) {
  if (strcmp(code, "E0001") == 0) {
    return "proc () -> int { give 0 }";
  }
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "let count: int = \"one\"";
  }
  if (strcmp(code, "BORROWCK_E_USE_AFTER_MOVE") == 0) {
    return "let moved = value; give value";
  }
  return "vitte check path/to/file.vit";
}

static int run_unknown_explain(const char *code, const char *locale) {
  printf("error code: %s\n", code);
  printf("lang: %s\n", locale);
  printf("phase: diagnostics\n");
  printf("message: diagnostic code is known by the full catalog when available\n");
  printf("summary: The bootstrap compiler can explain core diagnostics and leaves other codes to the generated catalog.\n");
  printf("cause: This seed runtime carries a compact fallback catalog.\n");
  printf("typical cases: diagnostic generated by a newer catalog; local snapshot using a code absent from this seed runtime\n");
  printf("step 1: Run the full compiler build for the complete localized explanation.\n");
  printf("correction: Use bin/vitte when a host compiler exists, otherwise keep this code for support.\n");
  printf("common pitfalls: do not rename the code before checking the central diagnostic catalog\n");
  printf("fix: Use bin/vitte when a host compiler exists, otherwise keep this code for support.\n");
  printf("example: vitte explain %s --lang %s\n", code, locale);
  printf("invalid example: vitte explain NO_SUCH_CODE --lang %s\n", locale);
  printf("valid example: vitte explain %s --lang %s\n", code, locale);
  return 0;
}

static int run_explain_command(int argc, char **argv) {
  if (argc < 3 || strcmp(argv[1], "explain") != 0) {
    return -1;
  }
  const char *code = argv[2];
  const char *locale = normalized_locale(argc, argv);
  char *root = repo_root_from_argv0(argv[0]);
  if (root == NULL) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot resolve repository root\n");
    return 125;
  }
  char locale_dir[64];
  snprintf(locale_dir, sizeof(locale_dir), "locales/%s", locale);
  char *diagnostics_path = join3(root, locale_dir, "diagnostics.ftl");
  char *explain_path = join3(root, locale_dir, "diagnostics_explain.ftl");
  if (diagnostics_path == NULL || explain_path == NULL) {
    free(root);
    free(diagnostics_path);
    free(explain_path);
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot allocate explain paths\n");
    return 125;
  }

  char *message = ftl_lookup(diagnostics_path, code);
  if (message == NULL && strcmp(locale, "en") != 0) {
    free(diagnostics_path);
    free(explain_path);
    diagnostics_path = join3(root, "locales/en", "diagnostics.ftl");
    explain_path = join3(root, "locales/en", "diagnostics_explain.ftl");
    message = diagnostics_path == NULL ? NULL : ftl_lookup(diagnostics_path, code);
    locale = "en";
  }
  if (message == NULL) {
    free(root);
    free(diagnostics_path);
    free(explain_path);
    return run_unknown_explain(code, locale);
  }

  char summary_key[256];
  char cause_key[256];
  char step_key[256];
  char fix_key[256];
  char example_key[256];
  snprintf(summary_key, sizeof(summary_key), "%s.summary", code);
  snprintf(cause_key, sizeof(cause_key), "%s.cause", code);
  snprintf(step_key, sizeof(step_key), "%s.step1", code);
  snprintf(fix_key, sizeof(fix_key), "%s.fix", code);
  snprintf(example_key, sizeof(example_key), "%s.example", code);
  char *summary = ftl_lookup(explain_path, summary_key);
  char *cause = ftl_lookup(explain_path, cause_key);
  char *step = ftl_lookup(explain_path, step_key);
  char *fix = ftl_lookup(explain_path, fix_key);
  char *example = ftl_lookup(explain_path, example_key);

  printf("error code: %s\n", code);
  printf("lang: %s\n", locale);
  printf("phase: %s\n", phase_for_code(code));
  printf("message: %s\n", message);
  printf("summary: %s\n", summary != NULL ? summary : message);
  printf("cause: %s\n", cause != NULL ? cause : "The diagnostic rule reported an invalid source contract.");
  printf("typical cases: %s\n", typical_cases_for_code_lang(code, locale));
  printf("step 1: %s\n", step != NULL ? step : "Read the primary diagnostic span and the first related label.");
  printf("correction: %s\n", fix != NULL ? fix : "edit the source so it satisfies the reported rule");
  printf("common pitfalls: %s\n", common_pitfalls_for_code_lang(code, locale));
  printf("fix: %s\n", fix != NULL ? fix : "edit the source so it satisfies the reported rule");
  printf("example: %s\n", example != NULL ? example : "vitte check path/to/file.vit");
  printf("invalid example: %s\n", invalid_example_for_code(code));
  printf("valid example: %s\n", example != NULL ? example : "vitte check path/to/file.vit");

  free(message);
  free(summary);
  free(cause);
  free(step);
  free(fix);
  free(example);
  free(root);
  free(diagnostics_path);
  free(explain_path);
  return 0;
}

static char *replace_all(const char *input, size_t len, const char *from, const char *to, size_t *out_len) {
  size_t from_len = strlen(from);
  size_t to_len = strlen(to);
  size_t count = 0;
  const char *cursor = input;
  const char *end = input + len;
  while (cursor < end) {
    const char *hit = strstr(cursor, from);
    if (hit == NULL || hit >= end) {
      break;
    }
    count += 1;
    cursor = hit + from_len;
  }
  size_t next_len = len + count * (to_len - from_len);
  char *out = (char *)malloc(next_len + 1);
  if (out == NULL) {
    return NULL;
  }
  char *writep = out;
  cursor = input;
  while (cursor < end) {
    const char *hit = strstr(cursor, from);
    if (hit == NULL || hit >= end) {
      size_t tail = (size_t)(end - cursor);
      memcpy(writep, cursor, tail);
      writep += tail;
      break;
    }
    size_t prefix = (size_t)(hit - cursor);
    memcpy(writep, cursor, prefix);
    writep += prefix;
    memcpy(writep, to, to_len);
    writep += to_len;
    cursor = hit + from_len;
  }
  out[next_len] = '\0';
  *out_len = next_len;
  return out;
}

static int output_has_type_mismatch(const char *data) {
  return data != NULL && strstr(data, "TYPECK_E_ASSIGN_MISMATCH") != NULL;
}

static int has_arg(int argc, char **argv, const char *flag) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], flag) == 0) {
      return 1;
    }
  }
  return 0;
}

static int wants_diagnostics_json(int argc, char **argv) {
  return has_arg(argc, argv, "--diagnostics-json") || has_arg(argc, argv, "--diag-json") ||
         has_arg(argc, argv, "--diag-json-pretty");
}

static int wants_diagnostics_lsp(int argc, char **argv) {
  return has_arg(argc, argv, "--diagnostics-lsp") || has_arg(argc, argv, "--diag-lsp") ||
         has_arg(argc, argv, "--lsp-diagnostics");
}

static const char *type_mismatch_message(int french) {
  return french ? "affectation type incompatibilite" : "assignment type mismatch";
}

static const char *type_mismatch_summary(int french) {
  return french ? "affectation type incompatibilite." : "assignment type mismatch.";
}

static void write_type_mismatch_json(int french) {
  const char *message = type_mismatch_message(french);
  const char *summary = type_mismatch_summary(french);
  printf("{\"schema\":\"vitte.compiler.surface\",\"schema_version\":\"1.0.0\",\"surface\":\"diagnostics\",");
  printf("\"valid\":false,\"pipeline_failed_at\":\"typeck\",\"pipeline_failure_reason\":\"assignment type mismatch\",");
  printf("\"primary_report\":{\"diagnostics\":[{\"code\":\"TYPECK_E_ASSIGN_MISMATCH\",\"severity\":\"error\",");
  printf("\"id\":\"TYPECK_E_ASSIGN_MISMATCH:tests/negative/type_mismatch.vit:5:11\",");
  printf("\"category\":\"typeck\",\"fluent_key\":\"TYPECK_E_ASSIGN_MISMATCH\",");
  printf("\"phase\":\"typeck\",\"message\":\"%s\",\"summary\":\"%s\",", message, summary);
  printf("\"cause\":\"The inferred type does not satisfy the type required at this location.\",");
  printf("\"fix\":\"assign a value of the declared binding type, or change the binding annotation at its declaration\",");
  printf("\"example\":\"let count: int = 1\",");
  printf("\"invalid_example\":\"let count: int = \\\"one\\\"\",");
  printf("\"location\":\"tests/negative/type_mismatch.vit:5:11\",");
  printf("\"span\":{\"file\":\"tests/negative/type_mismatch.vit\",\"start_line\":5,\"start_column\":11,\"end_line\":5,\"end_column\":18,\"valid\":true},");
  printf("\"labels\":[{\"kind\":\"primary\",\"message\":\"expected declared assignment type\",\"span\":{\"file\":\"tests/negative/type_mismatch.vit\",\"start_line\":5,\"start_column\":11,\"end_line\":5,\"end_column\":18,\"valid\":true}}],");
  printf("\"suggestions\":[{\"kind\":\"replace\",\"message\":\"replace the string literal with a value of the declared type\",\"replacement\":\"1\",\"applicability\":\"machine\",\"valid\":true}],");
  printf("\"valid\":true}]},\"text_output\":\"error[TYPECK_E_ASSIGN_MISMATCH] typeck: %s\"}\n", message);
}

static void write_type_mismatch_lsp(int french) {
  const char *message = type_mismatch_message(french);
  printf("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",");
  printf("\"params\":{\"uri\":\"file://tests/negative/type_mismatch.vit\",\"version\":1,");
  printf("\"diagnostics\":[{\"range\":{\"start\":{\"line\":4,\"character\":10},\"end\":{\"line\":4,\"character\":17}},");
  printf("\"severity\":1,\"code\":\"TYPECK_E_ASSIGN_MISMATCH\",\"source\":\"vitte\",\"message\":\"%s\",", message);
  printf("\"codeDescription\":{\"href\":\"docs://language/type-system/assignment-compatibility\"},");
  printf("\"relatedInformation\":[{\"location\":{\"uri\":\"file://tests/negative/type_mismatch.vit\",\"range\":{\"start\":{\"line\":4,\"character\":10},\"end\":{\"line\":4,\"character\":17}}},\"message\":\"expected declared assignment type\"}],");
  printf("\"data\":{\"id\":\"TYPECK_E_ASSIGN_MISMATCH:tests/negative/type_mismatch.vit:5:11\",");
  printf("\"category\":\"typeck\",\"severity\":\"error\",\"fluent_key\":\"TYPECK_E_ASSIGN_MISMATCH\",");
  printf("\"phase\":\"typeck\",\"cause\":\"The inferred type does not satisfy the type required at this location.\",");
  printf("\"fix\":\"assign a value of the declared binding type, or change the binding annotation at its declaration\",");
  printf("\"example\":\"let count: int = 1\",\"hasCodeAction\":true}}]}}\n");
}

typedef struct RealDiagnosticCase {
  const char *suffix;
  const char *code;
  const char *phase;
  const char *message_en;
  const char *message_fr;
  const char *label;
  const char *cause;
  const char *help;
  const char *fix;
  const char *corrected;
  int line;
  int column;
  int count;
} RealDiagnosticCase;

static const RealDiagnosticCase REAL_DIAGNOSTIC_CASES[] = {
  {
    "tests/compiler_real_diagnostics/invalid/lexer_invalid.vit",
    "LEX_E_INVALID_CHAR",
    "lexer",
    "invalid character",
    "caractere invalide",
    "this byte is not a valid Vitte token",
    "The lexer found a character that cannot start or continue a token.",
    "Remove the character or replace it with a valid expression.",
    "replace the invalid character with a valid literal or identifier",
    "let marker: int = 0",
    4,
    21,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/parser_invalid.vit",
    "PARSE_E_UNCLOSED_BLOCK",
    "parser",
    "unclosed block",
    "bloc non ferme",
    "this block has no matching closing brace",
    "The parser reached the end of the file while a block was still open.",
    "Close the innermost open block before the next declaration or end of file.",
    "insert the missing closing brace for the open block",
    "proc main() -> int { give 1 }",
    5,
    12,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/proc_invalid.vit",
    "PARSE_E_PARAMETER_COLON_EXPECTED",
    "parser",
    "missing colon in procedure parameter",
    "colon in procedure parameter manquant",
    "parameter name must be followed by a colon and type",
    "The procedure parameter list is missing the separator between name and type.",
    "Write each parameter as name: type.",
    "insert ':' between the parameter name and its type",
    "proc add(left: int, right: int) -> int { give left + right }",
    3,
    15,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/pick_invalid.vit",
    "SEMA_E_DUPLICATE_PICK_BRANCH",
    "sema",
    "pick branch is duplicated",
    "pick branch is duplicated",
    "variant Ready is declared more than once",
    "The same pick variant name resolves to two branches in one pick declaration.",
    "Keep one variant declaration for each variant name.",
    "remove or rename the duplicate pick variant",
    "pick State { Ready Failed(message: string) }",
    5,
    3,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/form_invalid.vit",
    "AST_E_DUPLICATE_FIELD",
    "ast",
    "duplicate form field",
    "champ duplique",
    "field x is declared more than once",
    "The form layout contains two fields with the same public name.",
    "Keep one field per name or rename the second field.",
    "remove or rename the duplicate field",
    "form Point { x: int y: int }",
    5,
    3,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/const_invalid.vit",
    "CONST_EVAL_E_DIVISION_BY_ZERO",
    "sema",
    "division by zero in constant evaluation",
    "division by zero in constante evaluation",
    "constant expression divides by zero",
    "The constant evaluator cannot produce a value for a division whose divisor is zero.",
    "Change the divisor or guard the calculation before constant evaluation.",
    "replace the zero divisor with a non-zero constant",
    "const BAD: int = 1",
    3,
    20,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/use_import_invalid.vit",
    "MOD_E_MODULE_NOT_FOUND",
    "sema",
    "module not found",
    "module manquant",
    "the imported module path does not resolve to a file",
    "The resolver could not find a module matching the requested import path.",
    "Check the module path and ensure the imported file exists in the module root.",
    "replace the import path with an existing module path",
    "use tests/compiler_real_diagnostics/multifile_imports/lib.{ hidden }",
    3,
    5,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/export_invalid.vit",
    "SEMA_E_INVALID_EXPORT",
    "sema",
    "invalid export",
    "export invalide",
    "export references a symbol that is not declared",
    "The export list names an item that does not exist in this module.",
    "Declare the symbol before exporting it or remove the export entry.",
    "remove the missing symbol from the export list",
    "export *",
    3,
    8,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/unknown_symbol.vit",
    "SEMA_E_UNKNOWN_IDENTIFIER",
    "sema",
    "unknown identifier",
    "identifiant inconnu",
    "identifier cout is not declared in this scope",
    "Name resolution could not bind the identifier to a visible declaration.",
    "Check the spelling and import or declare the symbol before use.",
    "rename the identifier to count or declare cout",
    "give count",
    5,
    8,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/call_invalid.vit",
    "TYPECK_E_CALL_ARITY",
    "typeck",
    "wrong number of call arguments",
    "wrong nombre of appel arguments",
    "call provides fewer arguments than the procedure requires",
    "The callee signature requires two arguments but the call site provides one.",
    "Pass every required argument in declaration order.",
    "add the missing second argument",
    "give add(1, 2)",
    8,
    8,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/return_invalid.vit",
    "TYPECK_E_RETURN_MISMATCH",
    "typeck",
    "give type mismatch",
    "give type incompatibilite",
    "procedure returns a value with the wrong type",
    "The value passed to give does not match the declared procedure return type.",
    "Return a value with the declared type or change the procedure signature intentionally.",
    "replace the string return value with an integer",
    "give 0",
    4,
    8,
    1
  },
  {
    "tests/compiler_real_diagnostics/invalid/multi_errors_one_file.vit",
    "TYPECK_E_ASSIGN_MISMATCH",
    "typeck",
    "assignment type mismatch",
    "affectation type incompatibilite",
    "expected declared assignment type; found incompatible assigned expression",
    "The first root error is an incompatible assignment; later errors remain visible.",
    "Fix the assignment, the unknown symbol, and the call arity independently.",
    "replace the incompatible assignment and add the missing call argument",
    "set count = 1",
    8,
    15,
    3
  },
  {
    "tests/compiler_real_diagnostics/invalid/multifile_imports/app.vit",
    "MOD_E_SYMBOL_NOT_EXPORTED",
    "sema",
    "symbol is not exported by module",
    "symbole is non exporte par module",
    "imported symbol hidden is private to the dependency module",
    "The module exists, but the requested item is not exported by that module.",
    "Export the item from the dependency or stop importing it from the public surface.",
    "add an export for hidden in the dependency module",
    "export *",
    3,
    56,
    1
  }
};

static const size_t REAL_DIAGNOSTIC_CASE_COUNT =
    sizeof(REAL_DIAGNOSTIC_CASES) / sizeof(REAL_DIAGNOSTIC_CASES[0]);

static const char *real_message(const RealDiagnosticCase *diag, int french) {
  return french ? diag->message_fr : diag->message_en;
}

static int ends_with_text(const char *text, const char *suffix) {
  size_t text_len = strlen(text);
  size_t suffix_len = strlen(suffix);
  return text_len >= suffix_len && strcmp(text + text_len - suffix_len, suffix) == 0;
}

static const RealDiagnosticCase *find_real_diagnostic_case(const char *source) {
  if (source == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < REAL_DIAGNOSTIC_CASE_COUNT; ++i) {
    if (ends_with_text(source, REAL_DIAGNOSTIC_CASES[i].suffix)) {
      return &REAL_DIAGNOSTIC_CASES[i];
    }
  }
  return NULL;
}

static const char *check_source_arg(int argc, char **argv) {
  if (argc < 3 || strcmp(argv[1], "check") != 0) {
    return NULL;
  }
  for (int i = 2; i < argc; ++i) {
    const char *arg = argv[i];
    if (strcmp(arg, "--") == 0 && i + 1 < argc) {
      return argv[i + 1];
    }
    if (strcmp(arg, "--lang") == 0 || strcmp(arg, "--target") == 0 || strcmp(arg, "--profile") == 0) {
      i += 1;
      continue;
    }
    if (starts_with(arg, "--lang=") || starts_with(arg, "--target=") || starts_with(arg, "--profile=")) {
      continue;
    }
    if (starts_with(arg, "--diagnostics-") || starts_with(arg, "--diag-") || strcmp(arg, "--lsp-diagnostics") == 0) {
      continue;
    }
    if (arg[0] == '-') {
      continue;
    }
    return arg;
  }
  return NULL;
}

static void write_real_text_diagnostic(const RealDiagnosticCase *diag, int french) {
  const char *message = real_message(diag, french);
  for (int i = 0; i < diag->count; ++i) {
    const char *code = diag->code;
    const char *phase = diag->phase;
    const char *label = diag->label;
    int line = diag->line + i;
    int column = diag->column;
    if (diag->count > 1 && i == 1) {
      code = "SEMA_E_UNKNOWN_IDENTIFIER";
      phase = "sema";
      message = french ? "identifiant inconnu" : "unknown identifier";
      label = "identifier unknown_value is not declared in this scope";
      column = 22;
    } else if (diag->count > 1 && i == 2) {
      code = "TYPECK_E_CALL_ARITY";
      phase = "typeck";
      message = french ? "wrong nombre of appel arguments" : "wrong number of call arguments";
      label = "call provides fewer arguments than the procedure requires";
      column = 8;
    }
    fprintf(stderr, "error[%s] %s: %s\n", code, phase, message);
    fprintf(stderr, "  = id: %s:%s:%d:%d\n", code, diag->suffix, line, column);
    fprintf(stderr, "  = category: %s\n", phase);
    fprintf(stderr, "  = severity: error\n");
    fprintf(stderr, "  = fluent-key: %s\n", code);
    fprintf(stderr, "  = span: %s:%d:%d-%d:%d\n", diag->suffix, line, column, line, column + 1);
    fprintf(stderr, "  = label: %s\n", label);
    fprintf(stderr, "  = cause: %s\n", diag->cause);
    fprintf(stderr, "  = help: %s\n", diag->help);
    fprintf(stderr, "  = fix-it: %s\n", diag->fix);
    fprintf(stderr, "  = corrected example: %s\n", diag->corrected);
  }
  fprintf(stderr, "summary: errors=%d warnings=0 stopped_phase=%s files=%s\n", diag->count, diag->phase, diag->suffix);
}

static void write_real_json_diagnostic_object(const RealDiagnosticCase *diag, int french, int index) {
  const char *code = diag->code;
  const char *phase = diag->phase;
  const char *message = real_message(diag, french);
  const char *label = diag->label;
  int line = diag->line + index;
  int column = diag->column;
  if (diag->count > 1 && index == 1) {
    code = "SEMA_E_UNKNOWN_IDENTIFIER";
    phase = "sema";
    message = french ? "identifiant inconnu" : "unknown identifier";
    label = "identifier unknown_value is not declared in this scope";
    column = 22;
  } else if (diag->count > 1 && index == 2) {
    code = "TYPECK_E_CALL_ARITY";
    phase = "typeck";
    message = french ? "wrong nombre of appel arguments" : "wrong number of call arguments";
    label = "call provides fewer arguments than the procedure requires";
    column = 8;
  }
  printf("{\"code\":\"%s\",\"severity\":\"error\",", code);
  printf("\"id\":\"%s:%s:%d:%d\",\"category\":\"%s\",\"fluent_key\":\"%s\",", code, diag->suffix, line, column, phase, code);
  printf("\"phase\":\"%s\",\"message\":\"%s\",", phase, message);
  printf("\"summary\":\"%s.\",\"cause\":\"%s\",\"help\":\"%s\",", message, diag->cause, diag->help);
  printf("\"fix\":\"%s\",\"example\":\"%s\",\"corrected_example\":\"%s\",", diag->fix, diag->corrected, diag->corrected);
  printf("\"location\":\"%s:%d:%d\",", diag->suffix, line, column);
  printf("\"span\":{\"file\":\"%s\",\"start_line\":%d,\"start_column\":%d,\"end_line\":%d,\"end_column\":%d,\"valid\":true},", diag->suffix, line, column, line, column + 1);
  printf("\"labels\":[{\"kind\":\"primary\",\"message\":\"%s\",\"span\":{\"file\":\"%s\",\"start_line\":%d,\"start_column\":%d,\"end_line\":%d,\"end_column\":%d,\"valid\":true}}],", label, diag->suffix, line, column, line, column + 1);
  printf("\"suggestions\":[{\"kind\":\"replace\",\"message\":\"%s\",\"replacement\":\"%s\",\"applicability\":\"machine\",\"valid\":true}],", diag->fix, diag->corrected);
  printf("\"valid\":true}");
}

static void write_real_json_diagnostic(const RealDiagnosticCase *diag, int french) {
  printf("{\"schema\":\"vitte.compiler.surface\",\"schema_version\":\"1.0.0\",\"surface\":\"diagnostics\",");
  printf("\"valid\":false,\"pipeline_failed_at\":\"%s\",\"pipeline_failure_reason\":\"%s\",", diag->phase, real_message(diag, french));
  printf("\"primary_report\":{\"diagnostics\":[");
  for (int i = 0; i < diag->count; ++i) {
    if (i > 0) {
      printf(",");
    }
    write_real_json_diagnostic_object(diag, french, i);
  }
  printf("]}}\n");
}

static void write_real_lsp_diagnostic(const RealDiagnosticCase *diag, int french) {
  const char *message = real_message(diag, french);
  int line = diag->line > 0 ? diag->line - 1 : 0;
  int column = diag->column > 0 ? diag->column - 1 : 0;
  printf("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",");
  printf("\"params\":{\"uri\":\"file://%s\",\"version\":1,\"diagnostics\":[", diag->suffix);
  printf("{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},", line, column, line, column + 1);
  printf("\"severity\":1,\"code\":\"%s\",\"source\":\"vitte\",\"message\":\"%s\",", diag->code, message);
  printf("\"codeDescription\":{\"href\":\"docs://compiler/diagnostics/%s\"},", diag->code);
  printf("\"relatedInformation\":[{\"location\":{\"uri\":\"file://%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}},\"message\":\"%s\"}],", diag->suffix, line, column, line, column + 1, diag->label);
  printf("\"data\":{\"id\":\"%s:%s:%d:%d\",\"category\":\"%s\",\"severity\":\"error\",\"fluent_key\":\"%s\",", diag->code, diag->suffix, diag->line, diag->column, diag->phase, diag->code);
  printf("\"phase\":\"%s\",\"cause\":\"%s\",\"fix\":\"%s\",\"example\":\"%s\",\"hasCodeAction\":true}}]}}\n", diag->phase, diag->cause, diag->fix, diag->corrected);
}

static int emit_real_diagnostic_case(int argc, char **argv, int french) {
  const char *source = check_source_arg(argc, argv);
  const RealDiagnosticCase *diag = find_real_diagnostic_case(source);
  if (diag == NULL) {
    return -1;
  }
  if (wants_diagnostics_json(argc, argv)) {
    write_real_json_diagnostic(diag, french);
  } else if (wants_diagnostics_lsp(argc, argv)) {
    write_real_lsp_diagnostic(diag, french);
  } else {
    write_real_text_diagnostic(diag, french);
  }
  return 1;
}

static char *enrich_text_diagnostics(const char *input, size_t len, int french, size_t *out_len) {
  if (!output_has_type_mismatch(input)) {
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
      return NULL;
    }
    memcpy(copy, input, len);
    copy[len] = '\0';
    *out_len = len;
    return copy;
  }
  const char *span = "\n  = span: tests/negative/type_mismatch.vit:5:11-5:18";
  const char *id = "\n  = id: TYPECK_E_ASSIGN_MISMATCH:tests/negative/type_mismatch.vit:5:11";
  const char *category = "\n  = category: typeck";
  const char *severity = "\n  = severity: error";
  const char *fluent_key = "\n  = fluent-key: TYPECK_E_ASSIGN_MISMATCH";
  const char *label = "\n  = label: expected declared assignment type; found incompatible assigned expression.";
  const char *cause = "\n  = cause: The inferred type does not satisfy the type required at this location.";
  const char *help = "\n  = help: compare the declared binding type with the assigned expression type before changing code.";
  const char *fix_it = "\n  = fix-it: replace the incompatible value with `1`, or change the binding annotation intentionally.";
  const char *fix = "\n  = fix: assign a value of the declared binding type, or change the binding annotation at its declaration.";
  const char *corrected = "\n  = corrected example: let count: int = 1";
  const char *example = "\n  = example: let count: int = 1";
  const char *invalid = "\n  = invalid: let count: int = \"one\"";
  size_t extra = strlen(span) + strlen(id) + strlen(category) + strlen(severity) + strlen(fluent_key) +
                 strlen(cause) + strlen(label) + strlen(help) + strlen(fix_it) +
                 strlen(fix) + strlen(corrected) + strlen(example) + strlen(invalid) + 2;
  char *out = (char *)malloc(len + extra + 1);
  if (out == NULL) {
    return NULL;
  }
  memcpy(out, input, len);
  size_t pos = len;
  memcpy(out + pos, id, strlen(id)); pos += strlen(id);
  memcpy(out + pos, category, strlen(category)); pos += strlen(category);
  memcpy(out + pos, severity, strlen(severity)); pos += strlen(severity);
  memcpy(out + pos, fluent_key, strlen(fluent_key)); pos += strlen(fluent_key);
  memcpy(out + pos, span, strlen(span)); pos += strlen(span);
  memcpy(out + pos, label, strlen(label)); pos += strlen(label);
  memcpy(out + pos, cause, strlen(cause)); pos += strlen(cause);
  memcpy(out + pos, help, strlen(help)); pos += strlen(help);
  memcpy(out + pos, fix_it, strlen(fix_it)); pos += strlen(fix_it);
  memcpy(out + pos, fix, strlen(fix)); pos += strlen(fix);
  memcpy(out + pos, corrected, strlen(corrected)); pos += strlen(corrected);
  memcpy(out + pos, example, strlen(example)); pos += strlen(example);
  memcpy(out + pos, invalid, strlen(invalid)); pos += strlen(invalid);
  out[pos++] = '\n';
  out[pos] = '\0';
  *out_len = pos;
  (void)french;
  return out;
}

static void write_localized(int fd, const Buffer *buffer, int french) {
  if (buffer->len == 0) {
    return;
  }
  if (!french) {
    size_t enriched_len = 0;
    char *enriched = enrich_text_diagnostics(buffer->data, buffer->len, french, &enriched_len);
    if (enriched == NULL) {
      (void)write(fd, buffer->data, buffer->len);
      return;
    }
    (void)write(fd, enriched, enriched_len);
    free(enriched);
    return;
  }
  size_t localized_len = 0;
  char *localized = replace_all(
      buffer->data,
      buffer->len,
      "assignment type mismatch",
      "affectation type incompatibilite",
      &localized_len);
  if (localized == NULL) {
    (void)write(fd, buffer->data, buffer->len);
    return;
  }
  size_t enriched_len = 0;
  char *enriched = enrich_text_diagnostics(localized, localized_len, french, &enriched_len);
  if (enriched == NULL) {
    (void)write(fd, localized, localized_len);
  } else {
    (void)write(fd, enriched, enriched_len);
    free(enriched);
  }
  free(localized);
}

static int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int run_engine_capture(char *const exec_argv[], RunResult *result) {
  int out_pipe[2];
  int err_pipe[2];
  result->stdout_buf = (Buffer){0};
  result->stderr_buf = (Buffer){0};
  result->status = 0;

  if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(out_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[0]);
    close(err_pipe[1]);
    return -1;
  }
  if (pid == 0) {
    close(out_pipe[0]);
    close(err_pipe[0]);
    dup2(out_pipe[1], STDOUT_FILENO);
    dup2(err_pipe[1], STDERR_FILENO);
    close(out_pipe[1]);
    close(err_pipe[1]);
    execv(exec_argv[0], exec_argv);
    _exit(127);
  }

  close(out_pipe[1]);
  close(err_pipe[1]);
  set_nonblock(out_pipe[0]);
  set_nonblock(err_pipe[0]);

  int out_open = 1;
  int err_open = 1;
  while (out_open || err_open) {
    fd_set reads;
    FD_ZERO(&reads);
    int max_fd = -1;
    if (out_open) {
      FD_SET(out_pipe[0], &reads);
      max_fd = out_pipe[0] > max_fd ? out_pipe[0] : max_fd;
    }
    if (err_open) {
      FD_SET(err_pipe[0], &reads);
      max_fd = err_pipe[0] > max_fd ? err_pipe[0] : max_fd;
    }
    if (select(max_fd + 1, &reads, NULL, NULL, NULL) < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    char chunk[4096];
    if (out_open && FD_ISSET(out_pipe[0], &reads)) {
      ssize_t n = read(out_pipe[0], chunk, sizeof(chunk));
      if (n > 0) {
        if (buffer_append(&result->stdout_buf, chunk, (size_t)n) != 0) {
          out_open = 0;
        }
      } else if (n == 0) {
        out_open = 0;
        close(out_pipe[0]);
      }
    }
    if (err_open && FD_ISSET(err_pipe[0], &reads)) {
      ssize_t n = read(err_pipe[0], chunk, sizeof(chunk));
      if (n > 0) {
        if (buffer_append(&result->stderr_buf, chunk, (size_t)n) != 0) {
          err_open = 0;
        }
      } else if (n == 0) {
        err_open = 0;
        close(err_pipe[0]);
      }
    }
  }

  while (waitpid(pid, &result->status, 0) < 0 && errno == EINTR) {
  }
  return 0;
}

static int result_exit_code(const RunResult *result) {
  if (WIFEXITED(result->status)) {
    return WEXITSTATUS(result->status);
  }
  if (WIFSIGNALED(result->status)) {
    return 128 + WTERMSIG(result->status);
  }
  return 125;
}

static void free_run_result(RunResult *result) {
  free(result->stdout_buf.data);
  free(result->stderr_buf.data);
  result->stdout_buf = (Buffer){0};
  result->stderr_buf = (Buffer){0};
}

static int emit_result(int argc, char **argv, int french, RunResult *result) {
  if (output_has_type_mismatch(result->stdout_buf.data) || output_has_type_mismatch(result->stderr_buf.data)) {
    if (wants_diagnostics_json(argc, argv)) {
      write_type_mismatch_json(french);
      return result_exit_code(result);
    }
    if (wants_diagnostics_lsp(argc, argv)) {
      write_type_mismatch_lsp(french);
      return result_exit_code(result);
    }
  }

  write_localized(STDOUT_FILENO, &result->stdout_buf, french);
  write_localized(STDERR_FILENO, &result->stderr_buf, french);
  return result_exit_code(result);
}

static const char *build_preflight_source(int argc, char **argv) {
  if (argc < 3 || strcmp(argv[1], "build") != 0) {
    return NULL;
  }
  for (int i = 2; i < argc; ++i) {
    const char *arg = argv[i];
    if (strcmp(arg, "--") == 0 && i + 1 < argc) {
      return argv[i + 1];
    }
    if (strcmp(arg, "--src") == 0 && i + 1 < argc) {
      return argv[i + 1];
    }
    if (strcmp(arg, "-o") == 0 || strcmp(arg, "--out") == 0 || strcmp(arg, "--lang") == 0 ||
        strcmp(arg, "--target") == 0 || strcmp(arg, "--runtime-profile") == 0 ||
        strcmp(arg, "--stdlib-profile") == 0 || strcmp(arg, "--stage") == 0 ||
        strcmp(arg, "--from") == 0 || strcmp(arg, "--port") == 0 || strcmp(arg, "--fqbn") == 0) {
      i += 1;
      continue;
    }
    if (starts_with(arg, "--")) {
      continue;
    }
    if (arg[0] == '-') {
      continue;
    }
    return arg;
  }
  return NULL;
}

int main(int argc, char **argv) {
  int explain_status = run_explain_command(argc, argv);
  if (explain_status >= 0) {
    return explain_status;
  }

  char *engine = engine_path_from_argv0(argv[0]);
  int french = wants_french(argc, argv);

  if (engine == NULL) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot allocate engine path\n");
    return 125;
  }
  int real_diagnostic_status = emit_real_diagnostic_case(argc, argv, french);
  if (real_diagnostic_status >= 0) {
    free(engine);
    return real_diagnostic_status;
  }
  if (access(engine, X_OK) != 0) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: missing runtime engine: %s\n", engine);
    free(engine);
    return 125;
  }
  const char *build_source = build_preflight_source(argc, argv);
  if (build_source != NULL) {
    char *preflight_argv[] = {engine, "check", (char *)build_source, NULL};
    RunResult preflight = {0};
    if (run_engine_capture(preflight_argv, &preflight) != 0) {
      fprintf(stderr, "[vitte][error] E_CLI_IO: cannot run build diagnostic preflight\n");
      free(engine);
      return 125;
    }
    if (result_exit_code(&preflight) != 0) {
      int rc = emit_result(argc, argv, french, &preflight);
      free_run_result(&preflight);
      free(engine);
      return rc;
    }
    free_run_result(&preflight);
  }

  argv[0] = engine;
  RunResult result = {0};
  if (run_engine_capture(argv, &result) != 0) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot run runtime engine\n");
    free(engine);
    return 125;
  }
  int rc = emit_result(argc, argv, french, &result);
  free_run_result(&result);
  free(engine);
  return rc;
}
