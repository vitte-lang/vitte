#include "vittec/diag/emitter.h"

void vittec_emit_options_init(vittec_emit_options_t* opt) {
  if (!opt) return;
  opt->out_stream = NULL;
  opt->json_pretty = 0;
  opt->json_one_per_line = 1;
}

void vittec_emit_human(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  (void)sm;
  (void)diags;
}

void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  (void)sm;
  (void)diags;
}

void vittec_emit_human_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt) {
  (void)sm;
  (void)diags;
  (void)opt;
}

void vittec_emit_json_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt) {
  (void)sm;
  (void)diags;
  (void)opt;
}

void vittec_emit_human_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  (void)sm;
  (void)bag;
}

void vittec_emit_json_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  (void)sm;
  (void)bag;
}

void vittec_emit_human_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt) {
  (void)sm;
  (void)bag;
  (void)opt;
}

void vittec_emit_json_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt) {
  (void)sm;
  (void)bag;
  (void)opt;
}
