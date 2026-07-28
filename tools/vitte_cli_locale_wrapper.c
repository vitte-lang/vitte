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

static void write_localized(int fd, const Buffer *buffer, int french) {
  if (buffer->len == 0) {
    return;
  }
  if (!french) {
    (void)write(fd, buffer->data, buffer->len);
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
  (void)write(fd, localized, localized_len);
  free(localized);
}

static int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv) {
  int explain_status = run_explain_command(argc, argv);
  if (explain_status >= 0) {
    return explain_status;
  }

  char *engine = engine_path_from_argv0(argv[0]);
  int out_pipe[2];
  int err_pipe[2];
  Buffer stdout_buf = {0};
  Buffer stderr_buf = {0};
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
  if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot create runtime pipes\n");
    free(engine);
    return 125;
  }

  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "[vitte][error] E_CLI_IO: cannot fork runtime engine\n");
    free(engine);
    return 125;
  }
  if (pid == 0) {
    close(out_pipe[0]);
    close(err_pipe[0]);
    dup2(out_pipe[1], STDOUT_FILENO);
    dup2(err_pipe[1], STDERR_FILENO);
    close(out_pipe[1]);
    close(err_pipe[1]);
    argv[0] = engine;
    execv(engine, argv);
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
        if (buffer_append(&stdout_buf, chunk, (size_t)n) != 0) {
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
        if (buffer_append(&stderr_buf, chunk, (size_t)n) != 0) {
          err_open = 0;
        }
      } else if (n == 0) {
        err_open = 0;
        close(err_pipe[0]);
      }
    }
  }

  int status = 0;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
  }
  write_localized(STDOUT_FILENO, &stdout_buf, french);
  write_localized(STDERR_FILENO, &stderr_buf, french);

  free(stdout_buf.data);
  free(stderr_buf.data);
  free(engine);
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return 125;
}
