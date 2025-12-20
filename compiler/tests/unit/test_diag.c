#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vittec/diag/diagnostic.h"
#include "vittec/diag/emitter.h"
#include "vittec/diag/source_map.h"

#define ASSERT_TRUE(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))

static void test_source_map_line_math(void) {
  printf("TEST: source_map_line_math\n");

  const char* src = "alpha\nbeta\r\ngamma";
  vittec_source_map_t sm;
  vittec_source_map_init(&sm);

  vittec_file_id_t file = UINT32_MAX;
  vittec_sv_t path = vittec_sv("mem://case", 10);
  int rc = vittec_source_map_add_memory(&sm, path, src, strlen(src), 1, &file);
  ASSERT_EQ(rc, VITTEC_SM_OK);
  ASSERT_EQ(vittec_source_map_file_count(&sm), 1u);

  vittec_line_col_t lc0 = vittec_source_map_line_col(&sm, file, 0);
  ASSERT_EQ(lc0.line, 1u);
  ASSERT_EQ(lc0.col, 1u);

  const char* beta = strstr(src, "beta");
  ASSERT_TRUE(beta != NULL);
  uint32_t beta_off = (uint32_t)(beta - src);
  vittec_line_col_t lc_beta = vittec_source_map_line_col(&sm, file, beta_off);
  ASSERT_EQ(lc_beta.line, 2u);
  ASSERT_EQ(lc_beta.col, 1u);

  const char* gamma = strstr(src, "gamma");
  ASSERT_TRUE(gamma != NULL);
  uint32_t gamma_off = (uint32_t)(gamma - src);
  vittec_line_col_t lc_gamma = vittec_source_map_line_col(&sm, file, gamma_off + 2u);
  ASSERT_EQ(lc_gamma.line, 3u);
  ASSERT_EQ(lc_gamma.col, 3u);

  uint32_t idx0 = vittec_source_map_line_index(&sm, file, 0u);
  uint32_t idx2 = vittec_source_map_line_index(&sm, file, gamma_off);
  ASSERT_EQ(idx0, 0u);
  ASSERT_EQ(idx2, 2u);

  vittec_span_t span;
  ASSERT_TRUE(vittec_source_map_line_span(&sm, file, 1u, &span));
  ASSERT_EQ(span.lo, beta_off);
  ASSERT_EQ(span.hi, beta_off + 4u);
  vittec_sv_t beta_line = vittec_source_map_line_text(&sm, file, 1u);
  ASSERT_EQ(beta_line.len, 4u);
  ASSERT_TRUE(strncmp(beta_line.data, "beta", beta_line.len) == 0);

  vittec_span_t line0_span;
  ASSERT_TRUE(vittec_source_map_line_span(&sm, file, 0u, &line0_span));
  ASSERT_EQ(line0_span.hi - line0_span.lo, 5u);

  vittec_source_map_free(&sm);
  ASSERT_EQ(vittec_source_map_file_count(&sm), 0u);
}

static void test_diag_bag_push_and_free(void) {
  printf("TEST: diag_bag_push_and_free\n");

  vittec_diag_bag_t bag;
  vittec_diag_bag_init(&bag);

  vittec_span_t primary = vittec_span(1u, 2u, 6u);
  vittec_diag_t diag;
  vittec_diag_init(&diag, VITTEC_SEV_ERROR, vittec_sv("E0001", 5), primary, vittec_sv("broken", 6));

  vittec_span_t secondary_span = vittec_span(1u, 6u, 9u);
  ASSERT_TRUE(vittec_diag_add_label(&diag, VITTEC_DIAG_LABEL_SECONDARY, secondary_span, vittec_sv("context", 7)));
  ASSERT_TRUE(vittec_diag_add_note(&diag, vittec_sv("fix it", 6)));
  vittec_diag_set_help(&diag, vittec_sv("hint", 4));

  ASSERT_TRUE(vittec_diag_bag_push(&bag, &diag));
  vittec_diag_free(&diag);

  ASSERT_EQ(bag.len, 1u);
  ASSERT_EQ(bag.errors, 1u);
  ASSERT_TRUE(vittec_diag_bag_has_errors(&bag));

  const vittec_diag_t* stored = &bag.diags[0];
  ASSERT_EQ(stored->labels_len, 2u);
  ASSERT_EQ(stored->notes_len, 1u);
  ASSERT_EQ(stored->help.len, 4u);
  ASSERT_EQ(stored->labels[0].style, VITTEC_DIAG_LABEL_PRIMARY);
  ASSERT_EQ(stored->labels[1].style, VITTEC_DIAG_LABEL_SECONDARY);

  vittec_diag_bag_free(&bag);
  ASSERT_EQ(bag.len, 0u);
  ASSERT_EQ(bag.diags, NULL);
}

static void test_diag_bag_push_new_cycle(void) {
  printf("TEST: diag_bag_push_new_cycle\n");

  vittec_diag_bag_t bag;
  vittec_diag_bag_init(&bag);

  vittec_diag_t* diag = vittec_diag_bag_push_new(
    &bag,
    VITTEC_SEV_WARNING,
    vittec_sv("W0100", 5),
    vittec_span(0u, 0u, 1u),
    vittec_sv("warn", 4)
  );
  ASSERT_TRUE(diag != NULL);
  ASSERT_EQ(bag.len, 1u);
  ASSERT_EQ(bag.errors, 0u);

  ASSERT_TRUE(vittec_diag_add_label(
    diag,
    VITTEC_DIAG_LABEL_SECONDARY,
    vittec_span(0u, 1u, 2u),
    vittec_sv("secondary", 9)
  ));
  ASSERT_TRUE(vittec_diag_add_note(diag, vittec_sv("note", 4)));
  vittec_diag_set_help(diag, vittec_sv("details", 7));

  vittec_diag_bag_free(&bag);
  ASSERT_EQ(bag.len, 0u);
}

static char* slurp_file(FILE* f, size_t* out_len) {
  if (out_len) *out_len = 0;
  if (!f) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) return NULL;
  long n = ftell(f);
  if (n < 0) return NULL;
  if (fseek(f, 0, SEEK_SET) != 0) return NULL;
  size_t len = (size_t)n;
  char* buf = (char*)malloc(len + 1u);
  if (!buf) return NULL;
  size_t rd = fread(buf, 1, len, f);
  buf[rd] = '\0';
  if (out_len) *out_len = rd;
  return buf;
}

static void test_emitters_smoke(void) {
  printf("TEST: emitters_smoke\n");

  const char* src = "alpha\nbeta\ngamma\n";
  vittec_source_map_t sm;
  vittec_source_map_init(&sm);

  vittec_file_id_t file = UINT32_MAX;
  vittec_sv_t path = vittec_sv("mem://emit", 10);
  ASSERT_EQ(vittec_source_map_add_memory(&sm, path, src, strlen(src), 1, &file), VITTEC_SM_OK);

  vittec_diag_bag_t bag;
  vittec_diag_bag_init(&bag);
  vittec_span_t sp = vittec_span(file, 6u, 10u); /* "beta" */
  vittec_diag_t* d = vittec_diag_error(&bag, vittec_sv("E0001", 5), sp, vittec_sv("broken", 6));
  ASSERT_TRUE(d != NULL);
  vittec_diag_set_help(d, vittec_sv("hint", 4));

  /* Human */
  FILE* hf = tmpfile();
  ASSERT_TRUE(hf != NULL);
  vittec_emit_options_t hop;
  vittec_emit_options_init(&hop);
  hop.out_stream = hf;
  hop.context_lines = 1;
  vittec_emit_human_bag_ex(&sm, &bag, &hop);
  fflush(hf);
  size_t hlen = 0;
  char* htxt = slurp_file(hf, &hlen);
  ASSERT_TRUE(htxt != NULL);
  ASSERT_TRUE(strstr(htxt, "error") != NULL);
  ASSERT_TRUE(strstr(htxt, "mem://emit:2:1") != NULL);
  ASSERT_TRUE(strstr(htxt, "beta") != NULL);
  free(htxt);
  fclose(hf);

  /* JSON */
  FILE* jf = tmpfile();
  ASSERT_TRUE(jf != NULL);
  vittec_emit_options_t jop;
  vittec_emit_options_init(&jop);
  jop.out_stream = jf;
  jop.json_one_per_line = 1;
  vittec_emit_json_bag_ex(&sm, &bag, &jop);
  fflush(jf);
  size_t jlen = 0;
  char* jtxt = slurp_file(jf, &jlen);
  ASSERT_TRUE(jtxt != NULL);
  ASSERT_TRUE(strstr(jtxt, "\"severity\":\"error\"") != NULL);
  ASSERT_TRUE(strstr(jtxt, "\"file\":\"mem://emit\"") != NULL);
  free(jtxt);
  fclose(jf);

  vittec_diag_bag_free(&bag);
  vittec_source_map_free(&sm);
}

int main(void) {
  test_source_map_line_math();
  test_diag_bag_push_and_free();
  test_diag_bag_push_new_cycle();
  test_emitters_smoke();
  printf("All diag/source_map tests passed.\n");
  return 0;
}
