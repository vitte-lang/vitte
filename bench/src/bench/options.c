#include "bench/options.h"
#include "bench/runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  options.c

  Command-line options parsing.
*/

void bench_print_usage(const char* program) {
  printf("Usage: %s [OPTIONS] [BENCHMARKS]\n\n", program);
  printf("Options:\n");
  printf("  -h, --help              Show this help message\n");
  printf("  --list                  List available benchmarks\n");
  printf("  --list-full             List with details (kind, etc.)\n");
  printf("  --all                   Run all registered benchmarks\n");
  printf("  --filter SUBSTR         Filter benchmarks by substring\n");
  printf("  --iters N               Micro: iterations per sample (default 1000000)\n");
  printf("  --samples N             Number of samples (default 7)\n");
  printf("  --seconds S             Macro: duration per sample (default 2.0)\n");
  printf("  --warmup N              Warmup iterations (default 1000)\n");
  printf("  --timecheck N           Macro: check clock every N iterations (default 256)\n");
  printf("  --csv FILE              Write results as CSV\n");
  printf("  --json FILE             Write results as JSON\n");
  printf("\nExamples:\n");
  printf("  %s --list\n", program);
  printf("  %s --iters 5000000 micro:add\n", program);
  printf("  %s --seconds 3 macro:json_parse\n", program);
  printf("  %s --all --filter hash\n", program);
}

int bench_parse_options(int argc, char** argv, bench_options_t* opts) {
  if (!opts) return 0;

  /* Initialize with defaults */
  memset(opts, 0, sizeof(*opts));
  opts->runner_config = (bench_runner_config_t)BENCH_RUNNER_CONFIG_DEFAULT;

  /* Allocate space for case names */
  opts->case_names = (const char**)malloc(argc * sizeof(const char*));
  if (!opts->case_names) return 0;

  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];

    if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      opts->show_help = 1;
    } else if (strcmp(arg, "--list") == 0) {
      opts->list_benchmarks = 1;
    } else if (strcmp(arg, "--list-full") == 0) {
      opts->list_full = 1;
    } else if (strcmp(arg, "--all") == 0) {
      opts->run_all = 1;
    } else if (strcmp(arg, "--filter") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.filter = argv[++i];
      }
    } else if (strcmp(arg, "--iters") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.iterations = (uint32_t)atoi(argv[++i]);
      }
    } else if (strcmp(arg, "--samples") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.samples = (uint32_t)atoi(argv[++i]);
      }
    } else if (strcmp(arg, "--seconds") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.duration_seconds = atof(argv[++i]);
      }
    } else if (strcmp(arg, "--warmup") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.warmup_count = (uint32_t)atoi(argv[++i]);
      }
    } else if (strcmp(arg, "--timecheck") == 0) {
      if (i + 1 < argc) {
        opts->runner_config.timecheck_freq = (uint32_t)atoi(argv[++i]);
      }
    } else if (strcmp(arg, "--csv") == 0) {
      if (i + 1 < argc) {
        opts->csv_file = argv[++i];
      }
    } else if (strcmp(arg, "--json") == 0) {
      if (i + 1 < argc) {
        opts->json_file = argv[++i];
      }
    } else if (arg[0] != '-') {
      /* Non-option argument: benchmark name */
      opts->case_names[opts->case_count++] = arg;
    }
  }

  return 1;
}
