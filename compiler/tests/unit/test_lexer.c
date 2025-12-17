#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "vittec/front/lexer.h"
#include "vittec/front/token.h"

#define ASSERT_TRUE(cond) assert(cond)
#define ASSERT_EQ(a, b) assert((a) == (b))

static size_t lex_with_options(
  const char* src,
  vittec_diag_sink_t* sink,
  const vittec_lexer_options_t* opt,
  vittec_token_t* out,
  size_t cap
) {
  vittec_lexer_t lx;
  uint32_t len = (uint32_t)strlen(src);
  if (opt) {
    vittec_lexer_init_ex(&lx, src, len, 1u, sink, NULL, opt);
  } else {
    vittec_lexer_init(&lx, src, len, 1u, sink);
  }
  size_t count = 0;
  for (;;) {
    ASSERT_TRUE(count < cap);
    out[count] = vittec_lex_next(&lx);
    count++;
    if (out[count - 1].kind == VITTEC_TOK_EOF) break;
  }
  return count;
}

static size_t lex_source(const char* src, vittec_token_t* out, size_t cap) {
  return lex_with_options(src, NULL, NULL, out, cap);
}

static void test_lexer_basic_sequence(void) {
  printf("TEST: lexer_basic_sequence\n");
  const char* src = "foo 42 \"hi\" + -";
  vittec_token_t toks[16];
  size_t count = lex_source(src, toks, 16);
  ASSERT_TRUE(count >= 6);

  ASSERT_EQ(toks[0].kind, VITTEC_TOK_IDENT);
  ASSERT_TRUE(vittec_sv_eq_cstr(toks[0].text, "foo"));

  ASSERT_EQ(toks[1].kind, VITTEC_TOK_INT);
  ASSERT_TRUE(vittec_sv_eq_cstr(toks[1].text, "42"));

  ASSERT_EQ(toks[2].kind, VITTEC_TOK_STRING);
  ASSERT_TRUE(vittec_sv_eq_cstr(toks[2].text, "\"hi\""));

  ASSERT_EQ(toks[3].kind, VITTEC_TOK_PUNCT);
  ASSERT_EQ(toks[3].as.punct, VITTEC_PUNCT_PLUS);

  ASSERT_EQ(toks[4].kind, VITTEC_TOK_PUNCT);
  ASSERT_EQ(toks[4].as.punct, VITTEC_PUNCT_MINUS);

  ASSERT_EQ(toks[5].kind, VITTEC_TOK_EOF);
}

static void test_lexer_keywords(void) {
  printf("TEST: lexer_keywords\n");
  const char* src = "fn let return end";
  vittec_token_t toks[16];
  size_t count = lex_source(src, toks, 16);
  ASSERT_TRUE(count >= 5);

  ASSERT_EQ(toks[0].kind, VITTEC_TOK_KEYWORD);
  ASSERT_EQ(toks[0].as.kw, VITTEC_KW_FN);

  ASSERT_EQ(toks[1].kind, VITTEC_TOK_KEYWORD);
  ASSERT_EQ(toks[1].as.kw, VITTEC_KW_LET);

  ASSERT_EQ(toks[2].kind, VITTEC_TOK_KEYWORD);
  ASSERT_EQ(toks[2].as.kw, VITTEC_KW_RETURN);

  ASSERT_EQ(toks[3].kind, VITTEC_TOK_KEYWORD);
  ASSERT_EQ(toks[3].as.kw, VITTEC_KW_END);
}

static void test_lexer_punctuation_suite(void) {
  printf("TEST: lexer_punctuation_suite\n");
  const char* src = "() [] , ; . : -> => == !=";
  vittec_token_t toks[32];
  size_t count = lex_source(src, toks, 32);
  ASSERT_TRUE(count >= 12);

  ASSERT_EQ(toks[0].kind, VITTEC_TOK_PUNCT);
  ASSERT_EQ(toks[0].as.punct, VITTEC_PUNCT_LPAREN);
  ASSERT_EQ(toks[1].as.punct, VITTEC_PUNCT_RPAREN);
  ASSERT_EQ(toks[2].as.punct, VITTEC_PUNCT_LBRACK);
  ASSERT_EQ(toks[3].as.punct, VITTEC_PUNCT_RBRACK);
  ASSERT_EQ(toks[4].as.punct, VITTEC_PUNCT_COMMA);
  ASSERT_EQ(toks[5].as.punct, VITTEC_PUNCT_SEMI);
  ASSERT_EQ(toks[6].as.punct, VITTEC_PUNCT_DOT);
  ASSERT_EQ(toks[7].as.punct, VITTEC_PUNCT_COLON);
  ASSERT_EQ(toks[8].as.punct, VITTEC_PUNCT_ARROW);
  ASSERT_EQ(toks[9].as.punct, VITTEC_PUNCT_FATARROW);
  ASSERT_EQ(toks[10].as.punct, VITTEC_PUNCT_EQEQ);
  ASSERT_EQ(toks[11].as.punct, VITTEC_PUNCT_NE);
}

static void test_lexer_newlines_and_comments(void) {
  printf("TEST: lexer_newlines_and_comments\n");
  const char* src = "a\n// comment\nb";
  vittec_lexer_options_t opt;
  vittec_lexer_options_init(&opt);
  opt.flags = VITTEC_LEX_KEEP_NEWLINES | VITTEC_LEX_KEEP_COMMENTS;
  vittec_token_t toks[16];
  size_t count = lex_with_options(src, NULL, &opt, toks, 16);
  ASSERT_TRUE(count >= 5);

  ASSERT_EQ(toks[0].kind, VITTEC_TOK_IDENT);
  ASSERT_EQ(toks[1].kind, VITTEC_TOK_NEWLINE);
  ASSERT_EQ(toks[2].kind, VITTEC_TOK_COMMENT);
  ASSERT_TRUE(vittec_sv_eq_cstr(toks[2].text, "// comment"));
  ASSERT_EQ(toks[3].kind, VITTEC_TOK_NEWLINE);
  ASSERT_EQ(toks[4].kind, VITTEC_TOK_IDENT);
}

static void test_lexer_reports_diagnostics(void) {
  printf("TEST: lexer_reports_diagnostics\n");
  vittec_diag_sink_t sink;
  vittec_diag_sink_init(&sink);

  vittec_token_t toks[8];
  size_t count = lex_with_options("#", &sink, NULL, toks, 8);
  ASSERT_TRUE(count >= 1);
  ASSERT_EQ(toks[count - 1].kind, VITTEC_TOK_EOF);

  ASSERT_EQ(sink.len, 1u);
  ASSERT_EQ(sink.diags[0].severity, VITTEC_SEV_ERROR);
  ASSERT_EQ(sink.diags[0].span.lo, 0u);

  vittec_diag_sink_free(&sink);
}

static void test_lexer_unterminated_block_comment_diag(void) {
  printf("TEST: lexer_unterminated_block_comment_diag\n");
  vittec_diag_sink_t sink;
  vittec_diag_sink_init(&sink);

  vittec_token_t toks[4];
  size_t count = lex_with_options("/* unterminated", &sink, NULL, toks, 4);
  ASSERT_TRUE(count >= 1);
  ASSERT_EQ(sink.len, 1u);
  ASSERT_TRUE(vittec_sv_eq_cstr(sink.diags[0].message, "unterminated block comment"));

  vittec_diag_sink_free(&sink);
}

static void test_lexer_unterminated_string_escape_diag(void) {
  printf("TEST: lexer_unterminated_string_escape_diag\n");
  vittec_diag_sink_t sink;
  vittec_diag_sink_init(&sink);

  const char* src = "\"escape\\";
  vittec_token_t toks[8];
  size_t count = lex_with_options(src, &sink, NULL, toks, 8);
  ASSERT_TRUE(count >= 1);
  ASSERT_EQ(sink.len, 1u);
  ASSERT_TRUE(vittec_sv_eq_cstr(sink.diags[0].message, "unterminated string literal"));

  vittec_diag_sink_free(&sink);
}

int main(void) {
  printf("=== vittec Lexer Unit Tests ===\n\n");
  test_lexer_basic_sequence();
  test_lexer_keywords();
  test_lexer_punctuation_suite();
  test_lexer_newlines_and_comments();
  test_lexer_reports_diagnostics();
  test_lexer_unterminated_block_comment_diag();
  test_lexer_unterminated_string_escape_diag();
  printf("\nAll vittec lexer tests passed.\n");
  return 0;
}
