#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vittec/diag/diagnostic.h"
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

int main(void) {
  test_source_map_line_math();
  test_diag_bag_push_and_free();
  test_diag_bag_push_new_cycle();
  printf("All diag/source_map tests passed.\n");
  return 0;
}
