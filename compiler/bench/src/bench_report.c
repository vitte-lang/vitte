\
/*
  bench_report.c - reporting in text and json
*/
#include "bench/bench.h"
#include <stdio.h>
#include <string.h>

static void json_escape(FILE *fp, const char *s) {
  for (const char *p = s ? s : ""; *p; ++p) {
    unsigned char c = (unsigned char)*p;
    if (c == '\"' || c == '\\') { fputc('\\', fp); fputc((char)c, fp); }
    else if (c < 0x20) fprintf(fp, "\\u%04x", (unsigned)c);
    else fputc((char)c, fp);
  }
}

void bench_report_text(FILE *fp, const bench_result *results, size_t count, const bench_opts *opts) {
  (void)opts;
  fprintf(fp, "%-10s  %-28s  %5s  %10s  %10s  %10s  %10s  %10s\n",
          "suite", "name", "iters", "mean(s)", "p50(s)", "p90(s)", "p99(s)", "min(s)", "max(s)");
  for (size_t i = 0; i < count; ++i) {
    const bench_result *r = &results[i];
    fprintf(fp, "%-10s  %-28s  %5u  %10.6f  %10.6f  %10.6f  %10.6f  %10.6f  %10.6f\n",
            r->suite, r->name, r->iters,
            r->mean_s, r->p50_s, r->p90_s, r->p99_s, r->min_s, r->max_s);
  }
}

void bench_report_json(FILE *fp, const bench_result *results, size_t count, const bench_opts *opts) {
  fprintf(fp, "{\"format\":\"vitte-bench\",\"count\":%zu,\"iters\":%u,\"results\":[",
          count, opts ? opts->iters : 0);

  for (size_t i = 0; i < count; ++i) {
    const bench_result *r = &results[i];
    if (i) fputc(',', fp);
    fputs("{\"suite\":\"", fp); json_escape(fp, r->suite);
    fputs("\",\"name\":\"", fp); json_escape(fp, r->name);
    fprintf(fp,
            "\",\"iters\":%u"
            ",\"total_s\":%.9f"
            ",\"mean_s\":%.9f"
            ",\"min_s\":%.9f"
            ",\"max_s\":%.9f"
            ",\"stddev_s\":%.9f"
            ",\"p50_s\":%.9f"
            ",\"p90_s\":%.9f"
            ",\"p99_s\":%.9f}",
            r->iters,
            r->total_s, r->mean_s, r->min_s, r->max_s, r->stddev_s, r->p50_s, r->p90_s, r->p99_s);
  }

  fputs("]}\n", fp);
}
