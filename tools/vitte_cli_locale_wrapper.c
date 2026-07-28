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
