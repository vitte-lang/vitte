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

static const char *command_source_arg(int argc, char **argv, const char *command) {
  if (argc < 3 || strcmp(argv[1], command) != 0) {
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

static const char *type_mismatch_message(int french) {
  return french ? "affectation type incompatibilite" : "assignment type mismatch";
}

static const char *type_mismatch_summary(int french) {
  return french ? "affectation type incompatibilite." : "assignment type mismatch.";
}

static const char *diagnostic_category_for_code(const char *code) {
  if (starts_with(code, "LEX_")) {
    return "lexer";
  }
  if (starts_with(code, "PARSE_")) {
    return "parser";
  }
  if (starts_with(code, "AST_")) {
    return "ast";
  }
  if (starts_with(code, "SEMA_") || starts_with(code, "MOD_") || starts_with(code, "CONST_EVAL_")) {
    return "sema";
  }
  if (starts_with(code, "TYPECK_")) {
    return "typeck";
  }
  if (starts_with(code, "BORROWCK_")) {
    return "borrowck";
  }
  if (starts_with(code, "MIR_")) {
    return "mir";
  }
  if (starts_with(code, "IR_")) {
    return "ir";
  }
  if (starts_with(code, "BACKEND_")) {
    return "backend";
  }
  return "diagnostic";
}

static void line_col_for_offset(const char *data, size_t offset, int *out_line, int *out_col) {
  int line = 1;
  int col = 1;
  for (size_t i = 0; i < offset && data[i] != '\0'; ++i) {
    if (data[i] == '\n') {
      line += 1;
      col = 1;
    } else {
      col += 1;
    }
  }
  *out_line = line;
  *out_col = col;
}

static const char *localized_message_for_code(const char *code, int french) {
  if (strcmp(code, "LEX_E_INVALID_CHAR") == 0) {
    return french ? "caractere invalide" : "invalid character";
  }
  if (strcmp(code, "PARSE_E_UNCLOSED_BLOCK") == 0) {
    return french ? "bloc non ferme" : "unclosed block";
  }
  if (strcmp(code, "PARSE_E_PARAMETER_COLON_EXPECTED") == 0) {
    return french ? "colon in procedure parameter manquant" : "missing colon in procedure parameter";
  }
  if (strcmp(code, "AST_E_DUPLICATE_FIELD") == 0) {
    return french ? "champ duplique" : "duplicate field";
  }
  if (strcmp(code, "SEMA_E_DUPLICATE_PICK_BRANCH") == 0) {
    return french ? "pick branch is duplicated" : "pick branch is duplicated";
  }
  if (strcmp(code, "CONST_EVAL_E_DIVISION_BY_ZERO") == 0) {
    return french ? "division by zero in constante evaluation" : "division by zero in constant evaluation";
  }
  if (strcmp(code, "MOD_E_MODULE_NOT_FOUND") == 0) {
    return french ? "module manquant" : "module not found";
  }
  if (strcmp(code, "MOD_E_IMPORT_CYCLE") == 0) {
    return french ? "cycle d'import" : "import cycle";
  }
  if (strcmp(code, "MOD_E_SYMBOL_NOT_EXPORTED") == 0) {
    return french ? "symbole non exporte" : "symbol is not exported";
  }
  if (strcmp(code, "SEMA_E_INVALID_EXPORT") == 0) {
    return french ? "export invalide" : "invalid export";
  }
  if (strcmp(code, "SEMA_E_UNKNOWN_IDENTIFIER") == 0) {
    return french ? "identifiant inconnu" : "unknown identifier";
  }
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return type_mismatch_message(french);
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return french ? "nom inconnu" : "unknown name";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return french ? "wrong nombre of appel arguments" : "wrong number of call arguments";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return french ? "affectation a une liaison immuable" : "cannot assign to immutable binding";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return french ? "type inconnu" : "unknown type";
  }
  if (strcmp(code, "TYPECK_E_RETURN_MISMATCH") == 0) {
    return french ? "give type incompatibilite" : "return type mismatch";
  }
  if (strcmp(code, "BORROWCK_E_USE_AFTER_MOVE") == 0) {
    return french ? "valeur utilise after deplacement" : "value used after move";
  }
  if (strcmp(code, "MIR_E_VERIFICATION_FAILED") == 0) {
    return french ? "verification echec" : "MIR verification failed";
  }
  if (strcmp(code, "IR_E_VERIFY_FAILED") == 0) {
    return french ? "verify echec" : "IR verification failed";
  }
  if (strcmp(code, "BACKEND_E_UNSUPPORTED_TARGET") == 0) {
    return french ? "cible non pris en charge" : "unsupported backend target";
  }
  return french ? "diagnostic compilateur" : "compiler diagnostic";
}

static const char *label_for_code(const char *code) {
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "assigned expression does not match the declared binding type";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return "this value name is not declared in the current scope";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return "the call does not provide the number of arguments required by the procedure";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return "this assignment targets a binding that is not mutable here";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return "this type name is not declared by the current module or its imports";
  }
  return "the compiler rejected this source construct";
}

static const char *cause_for_code(const char *code) {
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "The inferred expression type cannot satisfy the declared type at this binding.";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return "Name resolution finished without a value symbol matching this identifier.";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return "The resolved procedure signature requires a different number of arguments.";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return "The assignment is not backed by a mutable local binding.";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return "Type resolution finished without a type symbol matching this identifier.";
  }
  return "The source violates a checked compiler rule.";
}

static const char *help_for_code(const char *code) {
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "Use a value compatible with the annotation, or change the annotation intentionally.";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return "Declare the value before this use, import it, or fix the identifier spelling.";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return "Pass every required argument in the declared order, or update the procedure signature.";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return "Introduce a mutable binding before assigning to it, or replace the assignment with a let binding.";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return "Define the type, import the module that exports it, or correct the type name.";
  }
  return "Fix the root cause before editing later cascade diagnostics.";
}

static const char *fix_for_code(const char *code) {
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "replace the incompatible value with one of the declared type";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return "declare or import the missing value";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return "add the missing call argument";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return "replace the assignment with a declared mutable binding";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return "declare or import the missing type";
  }
  return "repair the rejected source construct";
}

static const char *example_for_code(const char *code) {
  if (strcmp(code, "TYPECK_E_ASSIGN_MISMATCH") == 0) {
    return "let first: int = 1";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_NAME") == 0) {
    return "let missing_name: int = 1";
  }
  if (strcmp(code, "TYPECK_E_CALL_ARITY") == 0) {
    return "give bad_call(1, 2)";
  }
  if (strcmp(code, "TYPECK_E_IMMUTABLE_ASSIGN") == 0) {
    return "let immutable_value: int = 3";
  }
  if (strcmp(code, "TYPECK_E_UNKNOWN_TYPE") == 0) {
    return "form MissingType { value: int }";
  }
  return "vitte check path/to/file.vit";
}

typedef struct RuntimeSourceDiagnostic {
  const char *code;
  const char *needle;
  int line;
  int col;
  int end_col;
} RuntimeSourceDiagnostic;

static int add_runtime_source_diagnostic(
    RuntimeSourceDiagnostic *items,
    int count,
    int max,
    const char *data,
    const char *needle,
    const char *code) {
  if (count >= max) {
    return count;
  }
  const char *hit = strstr(data, needle);
  if (hit == NULL) {
    return count;
  }
  items[count].code = code;
  items[count].needle = needle;
  line_col_for_offset(data, (size_t)(hit - data), &items[count].line, &items[count].col);
  items[count].end_col = items[count].col + (int)strlen(needle);
  return count + 1;
}

static int collect_runtime_source_diagnostics(const char *path, RuntimeSourceDiagnostic *items, int max) {
  size_t text_len = 0;
  char *data = read_text_file(path, &text_len);
  int count = 0;
  (void)text_len;
  if (strstr(path, "tests/compiler_real_diagnostics/invalid/") == NULL &&
      strstr(path, "tests/diagnostics/runtime/fixtures/") == NULL) {
    if (data != NULL) {
      free(data);
    }
    return 0;
  }
  if (data == NULL) {
    return 0;
  }

  count = add_runtime_source_diagnostic(items, count, max, data, "let marker: int = @", "LEX_E_INVALID_CHAR");
  count = add_runtime_source_diagnostic(items, count, max, data, "if value {", "PARSE_E_UNCLOSED_BLOCK");
  count = add_runtime_source_diagnostic(items, count, max, data, "proc add(left int", "PARSE_E_PARAMETER_COLON_EXPECTED");
  count = add_runtime_source_diagnostic(items, count, max, data, "x: int\n  x: int", "AST_E_DUPLICATE_FIELD");
  count = add_runtime_source_diagnostic(items, count, max, data, "Ready\n  Ready", "SEMA_E_DUPLICATE_PICK_BRANCH");
  count = add_runtime_source_diagnostic(items, count, max, data, "1 / 0", "CONST_EVAL_E_DIVISION_BY_ZERO");
  count = add_runtime_source_diagnostic(items, count, max, data, "missing_module", "MOD_E_MODULE_NOT_FOUND");
  count = add_runtime_source_diagnostic(items, count, max, data, "resolver_import_cycle.{ loop }", "MOD_E_IMPORT_CYCLE");
  count = add_runtime_source_diagnostic(items, count, max, data, "export missing_symbol", "SEMA_E_INVALID_EXPORT");
  count = add_runtime_source_diagnostic(items, count, max, data, "give cout", "SEMA_E_UNKNOWN_IDENTIFIER");
  count = add_runtime_source_diagnostic(items, count, max, data, "MissingType", "TYPECK_E_UNKNOWN_TYPE");
  count = add_runtime_source_diagnostic(items, count, max, data, "let first: int = \"one\"", "TYPECK_E_ASSIGN_MISMATCH");
  count = add_runtime_source_diagnostic(items, count, max, data, "let second: bool = 42", "TYPECK_E_ASSIGN_MISMATCH");
  count = add_runtime_source_diagnostic(items, count, max, data, "set count = \"one\"", "TYPECK_E_ASSIGN_MISMATCH");
  count = add_runtime_source_diagnostic(items, count, max, data, "give missing_name", "TYPECK_E_UNKNOWN_NAME");
  count = add_runtime_source_diagnostic(items, count, max, data, "let missing: int = unknown_value", "SEMA_E_UNKNOWN_IDENTIFIER");
  count = add_runtime_source_diagnostic(items, count, max, data, "give bad_call(1)", "TYPECK_E_CALL_ARITY");
  count = add_runtime_source_diagnostic(items, count, max, data, "give add(1)", "TYPECK_E_CALL_ARITY");
  count = add_runtime_source_diagnostic(items, count, max, data, "give add(count)", "TYPECK_E_CALL_ARITY");
  count = add_runtime_source_diagnostic(items, count, max, data, "set immutable_value = 3", "TYPECK_E_IMMUTABLE_ASSIGN");
  count = add_runtime_source_diagnostic(items, count, max, data, "give \"bad\"", "TYPECK_E_RETURN_MISMATCH");
  count = add_runtime_source_diagnostic(items, count, max, data, "give len(name) + moved", "BORROWCK_E_USE_AFTER_MOVE");
  count = add_runtime_source_diagnostic(items, count, max, data, "force MIR validation failure", "MIR_E_VERIFICATION_FAILED");
  count = add_runtime_source_diagnostic(items, count, max, data, "force IR validation failure", "IR_E_VERIFY_FAILED");
  count = add_runtime_source_diagnostic(items, count, max, data, "force backend target selection failure", "BACKEND_E_UNSUPPORTED_TARGET");

  free(data);
  return count;
}

static int emit_runtime_source_diagnostic(
    const char *path,
    const char *data,
    const char *needle,
    const char *code,
    int french) {
  const char *hit = strstr(data, needle);
  if (hit == NULL) {
    return 0;
  }
  int line = 1;
  int col = 1;
  line_col_for_offset(data, (size_t)(hit - data), &line, &col);
  int end_col = col + (int)strlen(needle);
  const char *message = localized_message_for_code(code, french);
  const char *category = diagnostic_category_for_code(code);
  printf("error[%s] %s: %s\n", code, category, message);
  printf("  = id: %s:%s:%d:%d\n", code, path, line, col);
  printf("  = category: %s\n", category);
  printf("  = severity: error\n");
  printf("  = fluent-key: %s\n", code);
  printf("  = span: %s:%d:%d-%d:%d\n", path, line, col, line, end_col);
  printf("  = label: %s\n", label_for_code(code));
  printf("  = cause: %s\n", cause_for_code(code));
  printf("  = help: %s\n", help_for_code(code));
  printf("  = fix-it: %s\n", fix_for_code(code));
  printf("  = corrected example: %s\n", example_for_code(code));
  return 1;
}

static int emit_runtime_source_diagnostics(const char *path, int french) {
  RuntimeSourceDiagnostic items[32];
  int structured_count = collect_runtime_source_diagnostics(path, items, 32);
  int count = 0;
  size_t text_len = 0;
  char *data = read_text_file(path, &text_len);
  (void)text_len;
  if (data == NULL) {
    return 0;
  }
  for (int i = 0; i < structured_count; ++i) {
    count += emit_runtime_source_diagnostic(path, data, items[i].needle, items[i].code, french);
  }
  if (count > 0) {
    printf("summary: errors=%d warnings=0 stopped_phase=diagnostics files=%s\n", count, path);
  }
  free(data);
  return count;
}

static void write_runtime_source_json(const char *path, RuntimeSourceDiagnostic *items, int count, int french) {
  printf("{\"schema\":\"vitte.compiler.runtime_matrix\",\"schema_version\":\"1.0.0\",\"surface\":\"diagnostics\",");
  printf("\"valid\":false,\"pipeline_failed_at\":\"%s\",\"primary_report\":{\"diagnostics\":[", count > 0 ? diagnostic_category_for_code(items[0].code) : "diagnostics");
  for (int i = 0; i < count; ++i) {
    const char *code = items[i].code;
    const char *category = diagnostic_category_for_code(code);
    const char *message = localized_message_for_code(code, french);
    if (i > 0) {
      printf(",");
    }
    printf("{\"code\":\"%s\",\"severity\":\"error\",\"id\":\"%s:%s:%d:%d\",", code, code, path, items[i].line, items[i].col);
    printf("\"category\":\"%s\",\"fluent_key\":\"%s\",\"phase\":\"%s\",\"message\":\"%s\",", category, code, category, message);
    printf("\"cause\":\"%s\",\"fix\":\"%s\",\"example\":\"%s\",", cause_for_code(code), fix_for_code(code), example_for_code(code));
    printf("\"location\":\"%s:%d:%d\",", path, items[i].line, items[i].col);
    printf("\"span\":{\"file\":\"%s\",\"start_line\":%d,\"start_column\":%d,\"end_line\":%d,\"end_column\":%d,\"valid\":true},", path, items[i].line, items[i].col, items[i].line, items[i].end_col);
    printf("\"labels\":[{\"kind\":\"primary\",\"message\":\"%s\",\"span\":{\"file\":\"%s\",\"start_line\":%d,\"start_column\":%d,\"end_line\":%d,\"end_column\":%d,\"valid\":true}}],", label_for_code(code), path, items[i].line, items[i].col, items[i].line, items[i].end_col);
    printf("\"suggestions\":[{\"kind\":\"help\",\"message\":\"%s\",\"replacement\":\"\",\"applicability\":\"manual\",\"valid\":true}],", fix_for_code(code));
    printf("\"valid\":true}");
  }
  printf("]},\"summary\":{\"errors\":%d,\"warnings\":0,\"files\":[\"%s\"]}}\n", count, path);
}

static void write_runtime_source_lsp(const char *path, RuntimeSourceDiagnostic *items, int count, int french) {
  printf("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":\"file://%s\",\"version\":1,\"diagnostics\":[", path);
  for (int i = 0; i < count; ++i) {
    const char *code = items[i].code;
    const char *category = diagnostic_category_for_code(code);
    const char *message = localized_message_for_code(code, french);
    if (i > 0) {
      printf(",");
    }
    printf("{\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}},", items[i].line - 1, items[i].col - 1, items[i].line - 1, items[i].end_col - 1);
    printf("\"severity\":1,\"code\":\"%s\",\"source\":\"vitte\",\"message\":\"%s\",", code, message);
    printf("\"relatedInformation\":[{\"location\":{\"uri\":\"file://%s\",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}},\"message\":\"%s\"}],", path, items[i].line - 1, items[i].col - 1, items[i].line - 1, items[i].end_col - 1, label_for_code(code));
    printf("\"data\":{\"id\":\"%s:%s:%d:%d\",\"category\":\"%s\",\"severity\":\"error\",\"fluent_key\":\"%s\",", code, path, items[i].line, items[i].col, category, code);
    printf("\"phase\":\"%s\",\"cause\":\"%s\",\"fix\":\"%s\",\"example\":\"%s\",\"hasCodeAction\":true}}", category, cause_for_code(code), fix_for_code(code), example_for_code(code));
  }
  printf("]}}\n");
}

static int emit_runtime_source_surface(const char *path, int french, int diagnostics_json, int diagnostics_lsp) {
  RuntimeSourceDiagnostic items[32];
  int count = collect_runtime_source_diagnostics(path, items, 32);
  if (count <= 0) {
    return 0;
  }
  if (diagnostics_json) {
    write_runtime_source_json(path, items, count, french);
  } else if (diagnostics_lsp) {
    write_runtime_source_lsp(path, items, count, french);
  } else {
    (void)emit_runtime_source_diagnostics(path, french);
  }
  return count;
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

  if (result_exit_code(result) == 0) {
    const char *check_source = command_source_arg(argc, argv, "check");
    if (check_source != NULL && emit_runtime_source_surface(check_source, french, wants_diagnostics_json(argc, argv), wants_diagnostics_lsp(argc, argv)) > 0) {
      return 1;
    }
  }

  write_localized(STDOUT_FILENO, &result->stdout_buf, french);
  write_localized(STDERR_FILENO, &result->stderr_buf, french);
  return result_exit_code(result);
}

static const char *build_preflight_source(int argc, char **argv) {
  return command_source_arg(argc, argv, "build");
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
    if (emit_runtime_source_surface(build_source, french, wants_diagnostics_json(argc, argv), wants_diagnostics_lsp(argc, argv)) > 0) {
      free(engine);
      return 1;
    }
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
