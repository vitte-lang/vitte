/* TODO: steel CLI */

// steel_main.c
//
// CLI front-end "steelc" (Vitte compiler driver) built on steel_args.c.
//
// Commands:
//   compile (default)   Compile one input file.
//   help                Show help.
//
// Examples:
//   steelc compile src/main.vitte --emit ir -O 2 -o out.ir
//   steelc src/main.vitte --emit c -o out.c
//   steelc @args.rsp
//
// Notes:
// - Response files (@file) are expanded by steel_args.c.
// - This driver is intentionally strict and deterministic in diagnostics.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "steel_compiler.h"

// -----------------------------------------------------------------------------
// Bootstrap-friendly declarations (mirror steel_args.c)
// -----------------------------------------------------------------------------

typedef enum steel_arg_kind {
  STEEL_ARG_BOOL = 0,
  STEEL_ARG_COUNT = 1,
  STEEL_ARG_STRING = 2,
  STEEL_ARG_INT = 3
} steel_arg_kind;

typedef struct steel_flag_spec {
  const char* long_name;
  char short_name;
  steel_arg_kind kind;
  void* out;               // bool*, int*, const char**
  const char* value_name;
  const char* help;
} steel_flag_spec;

typedef struct steel_args_diag {
  int code;
  char message[512];
} steel_args_diag;

typedef struct steel_args_result {
  const char* prog;
  const char* command;
  int command_index;
  int positional_count;
  const char** positionals;
} steel_args_result;

int steel_args_parse(int argc,
                     char** argv,
                     const steel_flag_spec* specs,
                     size_t specs_count,
                     steel_args_result* out,
                     steel_args_diag* d,
                     bool stop_at_nonflag);

void steel_args_free_result(steel_args_result* out);

void steel_args_print_usage(FILE* f,
                            const char* prog,
                            const char* header,
                            const char* commands,
                            const steel_flag_spec* specs,
                            size_t specs_count,
                            const char* footer);

const char* steel_args_diag_cstr(const steel_args_diag* d);

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void print_version(void) {
  puts("steelc 0.0.0-dev");
}

static steel_emit_kind parse_emit(const char* s) {
  if (!s || !*s) return STEEL_EMIT_NONE;
  if (strcmp(s, "ir") == 0)  return STEEL_EMIT_IR;
  if (strcmp(s, "c") == 0)   return STEEL_EMIT_C;
  if (strcmp(s, "asm") == 0) return STEEL_EMIT_ASM;
  if (strcmp(s, "obj") == 0) return STEEL_EMIT_OBJ;
  return STEEL_EMIT_NONE;
}

static int clamp_opt(int o) {
  if (o < 0) return 0;
  if (o > 3) return 3;
  return o;
}

static void usage(const char* prog, const steel_flag_spec* specs, size_t specs_count) {
  const char* header =
    "steelc — Vitte compiler driver\n";

  const char* commands =
    "  compile   Compile one input file (default)\n"
    "  help      Show this help\n";

  const char* footer =
    "Notes:\n"
    "  - Response files: steelc @args.rsp\n"
    "  - Emit kinds: ir | c | asm | obj\n"
    "\n"
    "Compile flags (after command or input):\n"
    "  -o, --out <PATH>         Output path\n"
    "  --emit <KIND>            ir|c|asm|obj\n"
    "  -O, --opt <N>            Optimization level 0..3\n"
    "  -g, --debug              Debug info\n"
    "  --target <TRIPLE>        Target triple\n"
    "  --toolchain <NAME>       clang|gcc|msvc|...\n"
    "  --workspace <PATH>       Workspace root\n"
    "  --werror                 Warnings as errors\n"
    "  --json                   JSON diagnostics/output when supported\n";

  steel_args_print_usage(stdout, prog, header, commands, specs, specs_count, footer);
}

static bool streq(const char* a, const char* b) {
  if (a == b) return true;
  if (!a || !b) return false;
  return strcmp(a, b) == 0;
}

static int die_usage(const char* prog, const steel_flag_spec* specs, size_t specs_count, const char* msg) {
  if (msg && *msg) {
    fprintf(stderr, "[steelc] error: %s\n\n", msg);
  }
  usage(prog, specs, specs_count);
  return 2;
}

static void print_compile_error(const steel_compile_diag* d, bool json) {
  const char* msg = (d && d->message[0]) ? d->message : "compile failed";
  int code = d ? d->code : 1;

  if (json) {
    printf("{\"kind\":\"compile_error\",\"schema_version\":1,\"data\":{");
    printf("\"code\":%d,\"message\":\"", code);
    for (const char* p = msg; *p; p++) {
      if (*p == '\\' || *p == '"') putchar('\\');
      putchar(*p);
    }
    printf("\"}}\n");
  } else {
    fprintf(stderr, "[steelc] error: %s\n", msg);
  }
}

// Parse compile args (flags + positionals). Returns 0 on success, 2 on usage error.
static int parse_compile_args(int argc, const char** argv,
                              steel_compile_opts* opts,
                              const char** out_input,
                              const char** out_err) {
  const char* input = NULL;

  for (int i = 0; i < argc; i++) {
    const char* a = argv[i];
    if (!a) a = "";

    if (streq(a, "--")) {
      // rest are positional
      for (int j = i + 1; j < argc; j++) {
        if (!input) input = argv[j];
        else {
          // ignore extra positionals for now (future: multiple units)
        }
      }
      break;
    }

    if (streq(a, "-o") || streq(a, "--out")) {
      if (i + 1 >= argc) { *out_err = "missing value for --out"; return 2; }
      opts->output_path = argv[++i];
      continue;
    }

    if (streq(a, "--emit")) {
      if (i + 1 >= argc) { *out_err = "missing value for --emit"; return 2; }
      opts->emit = parse_emit(argv[++i]);
      if (opts->emit == STEEL_EMIT_NONE) { *out_err = "invalid --emit (expected: ir|c|asm|obj)"; return 2; }
      continue;
    }

    if (streq(a, "-O") || streq(a, "--opt")) {
      if (i + 1 >= argc) { *out_err = "missing value for -O/--opt"; return 2; }
      opts->opt_level = clamp_opt(atoi(argv[++i]));
      continue;
    }

    if (streq(a, "-g") || streq(a, "--debug")) {
      opts->debug_info = true;
      continue;
    }

    if (streq(a, "--target")) {
      if (i + 1 >= argc) { *out_err = "missing value for --target"; return 2; }
      opts->target = argv[++i];
      continue;
    }

    if (streq(a, "--toolchain")) {
      if (i + 1 >= argc) { *out_err = "missing value for --toolchain"; return 2; }
      opts->toolchain = argv[++i];
      continue;
    }

    if (streq(a, "--workspace")) {
      if (i + 1 >= argc) { *out_err = "missing value for --workspace"; return 2; }
      opts->workspace_root = argv[++i];
      continue;
    }

    if (streq(a, "--werror")) {
      opts->werror = true;
      continue;
    }

    if (streq(a, "--json")) {
      opts->json_diag = true;
      continue;
    }

    if (streq(a, "-v") || streq(a, "--verbose")) {
      opts->verbose++;
      continue;
    }

    if (a[0] == '-' && a[1] != '\0') {
      *out_err = "unknown compile flag";
      return 2;
    }

    // positional
    if (!input) input = a;
    else {
      // ignore extra positionals for now
    }
  }

  if (!input || !*input) {
    *out_err = "missing input file";
    return 2;
  }

  *out_input = input;
  return 0;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main(int argc, char** argv) {
  // Global flags
  bool help = false;
  bool version = false;
  bool debug = false;
  bool werror = false;
  bool json = false;
  int verbose = 0;
  int opt = 2;

  const char* out_path = NULL;
  const char* emit = "ir";
  const char* target = NULL;
  const char* toolchain = NULL;
  const char* workspace = NULL;

  steel_flag_spec global_specs[] = {
    { "help",      'h', STEEL_ARG_BOOL,   &help,     NULL,   "Show help" },
    { "version",   'V', STEEL_ARG_BOOL,   &version,  NULL,   "Print version" },
    { "debug",     'g', STEEL_ARG_BOOL,   &debug,    NULL,   "Enable debug info" },
    { "werror",     0,  STEEL_ARG_BOOL,   &werror,   NULL,   "Warnings as errors" },
    { "json",       0,  STEEL_ARG_BOOL,   &json,     NULL,   "JSON diagnostics/output" },
    { "verbose",   'v', STEEL_ARG_COUNT,  &verbose,  NULL,   "Increase verbosity" },
    { "opt",       'O', STEEL_ARG_INT,    &opt,      "N",   "Optimization level 0..3" },
    { "out",       'o', STEEL_ARG_STRING, &out_path, "PATH","Output path" },
    { "emit",       0,  STEEL_ARG_STRING, &emit,     "KIND","Emit kind: ir|c|asm|obj" },
    { "target",     0,  STEEL_ARG_STRING, &target,   "TRIPLE", "Target triple" },
    { "toolchain",  0,  STEEL_ARG_STRING, &toolchain, "NAME", "Toolchain name" },
    { "workspace",  0,  STEEL_ARG_STRING, &workspace, "PATH", "Workspace root" }
  };

  steel_args_diag ad;
  steel_args_result ar;

  int prc = steel_args_parse(argc, argv,
                            global_specs, sizeof(global_specs) / sizeof(global_specs[0]),
                            &ar, &ad,
                            true /* stop_at_nonflag */);

  if (prc != 0) {
    return die_usage((argc > 0 && argv && argv[0]) ? argv[0] : "steelc",
                     global_specs, sizeof(global_specs) / sizeof(global_specs[0]),
                     steel_args_diag_cstr(&ad));
  }

  if (version) {
    print_version();
    steel_args_free_result(&ar);
    return 0;
  }

  // Command resolution:
  // - If command is help/compile => command mode.
  // - Otherwise, treat command token as input path (default compile).
  const char* cmd = ar.command;
  bool cmd_is_help = (cmd && streq(cmd, "help"));
  bool cmd_is_compile = (cmd && streq(cmd, "compile"));

  if (help || cmd_is_help || !cmd) {
    usage(ar.prog ? ar.prog : "steelc", global_specs, sizeof(global_specs) / sizeof(global_specs[0]));
    steel_args_free_result(&ar);
    return 0;
  }

  // Build compile argv view
  const char* carg0 = NULL;
  const char** cargv = NULL;
  int cargc = 0;

  if (cmd_is_compile) {
    cargv = ar.positionals;
    cargc = ar.positional_count;
  } else {
    // default compile: cmd is actually the input
    carg0 = cmd;
    cargc = ar.positional_count + 1;
    cargv = (const char**)malloc((size_t)cargc * sizeof(const char*));
    if (!cargv) {
      steel_args_free_result(&ar);
      fprintf(stderr, "[steelc] error: out of memory\n");
      return 1;
    }
    cargv[0] = carg0;
    for (int i = 0; i < ar.positional_count; i++) cargv[i + 1] = ar.positionals[i];
  }

  // Compose compile options from globals (then allow compile flags to override).
  steel_compile_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.emit = parse_emit(emit);
  if (opts.emit == STEEL_EMIT_NONE) opts.emit = STEEL_EMIT_IR;
  opts.opt_level = clamp_opt(opt);
  opts.debug_info = debug;
  opts.werror = werror;
  opts.json_diag = json;
  opts.verbose = verbose;
  opts.output_path = out_path;
  opts.target = target;
  opts.toolchain = toolchain;
  opts.workspace_root = workspace;

  const char* input = NULL;
  const char* perr = NULL;
  int crc = parse_compile_args(cargc, cargv, &opts, &input, &perr);
  if (!cmd_is_compile && cargv) free((void*)cargv);

  if (crc != 0) {
    int rc = die_usage(ar.prog ? ar.prog : "steelc",
                       global_specs, sizeof(global_specs) / sizeof(global_specs[0]),
                       perr ? perr : "invalid arguments");
    steel_args_free_result(&ar);
    return rc;
  }

  opts.input_path = input;

  steel_compile_diag cd;
  cd.code = 0;
  cd.message[0] = '\0';

  int rc = steel_compiler_compile(&opts, &cd);
  if (rc != 0) {
    print_compile_error(&cd, opts.json_diag);
    steel_args_free_result(&ar);
    return rc;
  }

  steel_args_free_result(&ar);
  return 0;
}