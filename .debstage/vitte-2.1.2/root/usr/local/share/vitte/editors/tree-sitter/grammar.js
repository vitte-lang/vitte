// Tree-sitter grammar for VITTE
// NOTE: keep syntax permissive; semantic checks stay in compiler.

module.exports = grammar({
  name: 'vitte',

  extras: $ => [
    /[ \t\r\n\f]/,
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.identifier,

  rules: {
    source_file: $ => repeat($._item),

    _item: $ => choice(
      $.contract_block,
      $.use_stmt,
      $.proc_decl,
      $.form_decl,
      $.pick_decl,
      $.trait_decl,
      $.entry_decl,
      $.let_stmt,
      $.if_stmt,
      $.give_stmt,
      $.expr_stmt,
    ),

    contract_block: $ => seq(
      '<<<',
      'ROLE-CONTRACT',
      repeat(choice($.identifier, $.diag_code, $.number_literal, $.string_literal, /[^<>\n]+/, /\n/)),
      '>>>'
    ),

    use_stmt: $ => seq(
      'use',
      field('path', $.scoped_identifier),
      'as',
      field('alias', choice($.alias_pkg, $.identifier))
    ),

    proc_decl: $ => seq('proc', field('name', choice($.api_identifier, $.identifier)), optional($.param_list), optional($.block)),
    form_decl: $ => seq('form', field('name', $.identifier), optional($.block)),
    pick_decl: $ => seq('pick', field('name', $.identifier), optional($.block)),
    trait_decl: $ => seq('trait', field('name', $.identifier), optional($.block)),
    entry_decl: $ => seq('entry', field('name', $.identifier), optional(seq('at', $.scoped_identifier)), optional($.block)),

    param_list: $ => seq('(', optional(seq($.identifier, repeat(seq(',', $.identifier)))), ')'),

    block: $ => seq('{', repeat($._stmt), '}'),

    _stmt: $ => choice(
      $.let_stmt,
      $.if_stmt,
      $.give_stmt,
      $.expr_stmt,
      $.use_stmt,
    ),

    let_stmt: $ => seq('let', field('name', choice($.alias_pkg, $.identifier)), optional(seq(':', $.type_identifier)), '=', $.expression),
    if_stmt: $ => seq('if', $.expression, $.block, optional(seq('else', $.block))),
    give_stmt: $ => seq('give', optional($.expression)),
    expr_stmt: $ => $.expression,

    expression: $ => choice(
      $.call_expr,
      $.binary_expr,
      $.unary_expr,
      $.scoped_identifier,
      $.identifier,
      $.api_identifier,
      $.diag_code,
      $.number_literal,
      $.string_literal,
      $.boolean_literal,
      $.null_literal,
      $.block,
      seq('(', $.expression, ')')
    ),

    call_expr: $ => prec(2, seq(
      field('callee', choice($.scoped_identifier, $.identifier, $.api_identifier)),
      '(',
      optional(seq($.expression, repeat(seq(',', $.expression)))),
      ')'
    )),

    unary_expr: $ => prec(3, seq(choice('not', '-'), $.expression)),
    binary_expr: $ => prec.left(1, seq($.expression, choice('and', 'or', 'is', '==', '!=', '>', '<', '>=', '<=', '+', '-', '*', '/'), $.expression)),

    scoped_identifier: $ => seq($.identifier, repeat1(seq(choice('/', '::', '.'), $.identifier))),

    type_identifier: $ => choice(
      'bool', 'string', 'int', 'char',
      'i8', 'i16', 'i32', 'i64', 'isize',
      'u8', 'u16', 'u32', 'u64', 'usize',
      'f32', 'f64',
      $.scoped_identifier,
      $.identifier
    ),

    boolean_literal: $ => choice('true', 'false'),
    null_literal: $ => 'null',

    identifier: _ => /[A-Za-z_][A-Za-z0-9_]*/,
    alias_pkg: _ => /[A-Za-z_][A-Za-z0-9_]*_pkg/,
    api_identifier: _ => /(diagnostics_|quickfix_|doctor_)[A-Za-z0-9_]*/,
    diag_code: _ => /VITTE-[A-Z]+[0-9]{4}/,
    number_literal: _ => /0x[0-9A-Fa-f]+|0b[01]+|[0-9]+/,
    string_literal: _ => /"([^"\\]|\\.)*"/,

    line_comment: _ => token(seq('//', /.*/)),
    block_comment: _ => token(seq('/*', /[^*]*\*+([^/*][^*]*\*+)*/, '/')),
  }
});
