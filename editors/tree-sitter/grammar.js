// Tree-sitter grammar for VITTE
// Ultra parser-oriented version.
// Keeps syntax permissive.
// Semantic validation belongs to compiler passes.
//
// Features:
// - attributes
// - async/runtime syntax
// - MIR/compiler terminology
// - generic types
// - closures
// - ranges
// - advanced expressions
// - declarations
// - import/export system
// - compiler-oriented blocks
// - ownership syntax
// - low-level syntax
// - modern precedence model

const PREC = {
  assign: 1,
  logic_or: 2,
  logic_and: 3,
  equality: 4,
  compare: 5,
  range: 6,
  additive: 7,
  multiplicative: 8,
  unary: 9,
  call: 10,
  member: 11,
};

module.exports = grammar({
  name: 'vitte',

  extras: $ => [
    /[\s\f\r\t\n]/,
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.identifier,

  conflicts: $ => [
    [$.scoped_identifier],
    [$.expression, $.scoped_identifier],
    [$.block, $.map_literal],
    [$.proc_decl],
    [$.flow_decl],
    [$.form_decl],
    [$.class_decl],
    [$.pick_decl],
    [$.enum_decl],
    [$.union_decl],
    [$.trait_decl],
    [$.entry_decl],
    [$.return_stmt],
    [$.give_stmt],
    [$.emit_stmt],
    [$._stmt, $.expression],
    [$.expression, $.closure_expr],
    [$.match_stmt, $.match_expr],
    [$.expression, $.if_expr],
    [$.if_stmt, $.expression, $.if_expr],
    [$.tuple_literal],
    [$.match_arm, $.expression],
  ],

  supertypes: $ => [
    $._item,
    $._stmt,
    $.expression,
    $.literal,
  ],

  rules: {

    //////////////////////////////////////////////////////////////////////////
    // SOURCE
    //////////////////////////////////////////////////////////////////////////

    source_file: $ => repeat($._item),

    _item: $ => choice(
      $.contract_block,
      $.space_decl,
      $.use_stmt,
      $.share_stmt,
      $.const_decl,
      $.global_decl,
      $.proc_decl,
      $.flow_decl,
      $.form_decl,
      $.class_decl,
      $.pick_decl,
      $.trait_decl,
      $.impl_decl,
      $.enum_decl,
      $.union_decl,
      $.entry_decl,
      $.macro_decl,
      $.expr_stmt,
    ),

    //////////////////////////////////////////////////////////////////////////
    // COMMENTS
    //////////////////////////////////////////////////////////////////////////

    line_comment: _ =>
      token(seq('//', /.*/)),

    block_comment: _ =>
      token(seq(
        '/*',
        /[^*]*\*+([^/*][^*]*\*+)*/,
        '/'
      )),

    region_comment: _ =>
      token(seq(
        '<<<',
        /(.|\n)*?/,
        '>>>'
      )),

    //////////////////////////////////////////////////////////////////////////
    // REGION CONTRACTS
    //////////////////////////////////////////////////////////////////////////

    contract_block: $ => seq(
      '<<<',
      repeat(choice(
        $.identifier,
        $.diag_code,
        $.number_literal,
        $.string_literal,
        /[^<>]+/
      )),
      '>>>'
    ),

    //////////////////////////////////////////////////////////////////////////
    // ATTRIBUTES
    //////////////////////////////////////////////////////////////////////////

    attribute: $ => seq(
      '#[',
      $.identifier,
      optional(seq(
        '(',
        optional(commaSep($.expression)),
        ')'
      )),
      ']'
    ),

    //////////////////////////////////////////////////////////////////////////
    // MODULE SYSTEM
    //////////////////////////////////////////////////////////////////////////

    space_decl: $ => seq(
      'space',
      field('path', $.scoped_identifier)
    ),

    use_stmt: $ => seq(
      choice('use', 'pull', 'import'),
      field('path', $.scoped_identifier),
      optional(seq(
        'as',
        field('alias', $.identifier)
      )),
      optional($._semicolon)
    ),

    share_stmt: $ => seq(
      choice('share', 'export'),
      field('path', choice(
        'all',
        $.identifier,
        $.scoped_identifier
      )),
      optional($._semicolon)
    ),

    //////////////////////////////////////////////////////////////////////////
    // GLOBALS
    //////////////////////////////////////////////////////////////////////////

    const_decl: $ => seq(
      'const',
      field('name', $.identifier),
      optional(seq(':', $.type_expression)),
      '=',
      $.expression,
      optional($._semicolon)
    ),

    global_decl: $ => seq(
      choice('global', 'static'),
      field('name', $.identifier),
      optional(seq(':', $.type_expression)),
      optional(seq('=', $.expression)),
      optional($._semicolon)
    ),

    //////////////////////////////////////////////////////////////////////////
    // DECLARATIONS
    //////////////////////////////////////////////////////////////////////////

    proc_decl: $ => seq(
      repeat($.attribute),
      optional(choice('async', 'unsafe', 'extern')),
      'proc',
      field('name', choice($.api_identifier, $.identifier)),
      optional($.generic_params),
      optional($.param_list),
      optional(seq(
        choice('->', 'gives'),
        $.type_expression
      )),
      choice(
        $.block,
        optional($._semicolon)
      )
    ),

    flow_decl: $ => seq(
      repeat($.attribute),
      'flow',
      field('name', $.identifier),
      optional($.generic_params),
      optional($.param_list),
      optional($.block)
    ),

    form_decl: $ => seq(
      repeat($.attribute),
      choice('form', 'struct'),
      field('name', $.identifier),
      optional($.generic_params),
      optional($.block)
    ),

    class_decl: $ => seq(
      repeat($.attribute),
      'class',
      field('name', $.identifier),
      optional($.generic_params),
      optional($.block)
    ),

    pick_decl: $ => seq(
      repeat($.attribute),
      'pick',
      field('name', $.identifier),
      optional($.generic_params),
      optional($.block)
    ),

    enum_decl: $ => seq(
      repeat($.attribute),
      'enum',
      field('name', $.identifier),
      optional($.block)
    ),

    union_decl: $ => seq(
      repeat($.attribute),
      'union',
      field('name', $.identifier),
      optional($.block)
    ),

    trait_decl: $ => seq(
      repeat($.attribute),
      choice('trait', 'interface'),
      field('name', $.identifier),
      optional($.generic_params),
      optional($.block)
    ),

    impl_decl: $ => seq(
      repeat($.attribute),
      'impl',
      optional($.generic_params),
      field('target', $.type_expression),
      optional(seq(
        'for',
        $.type_expression
      )),
      $.block
    ),

    macro_decl: $ => seq(
      'macro',
      field('name', $.identifier),
      optional($.param_list),
      $.block
    ),

    entry_decl: $ => seq(
      'entry',
      field('name', $.identifier),
      optional(seq(
        'at',
        field('target', $.scoped_identifier)
      )),
      optional($.block)
    ),

    //////////////////////////////////////////////////////////////////////////
    // PARAMETERS
    //////////////////////////////////////////////////////////////////////////

    param_list: $ => seq(
      '(',
      optional(commaSep($.parameter)),
      ')'
    ),

    parameter: $ => seq(
      field('name', $.identifier),
      optional(seq(':', $.type_expression))
    ),

    generic_params: $ => seq(
      '<',
      commaSep1($.identifier),
      '>'
    ),

    //////////////////////////////////////////////////////////////////////////
    // BLOCKS
    //////////////////////////////////////////////////////////////////////////

    block: $ => seq(
      '{',
      repeat($._stmt),
      '}'
    ),

    //////////////////////////////////////////////////////////////////////////
    // STATEMENTS
    //////////////////////////////////////////////////////////////////////////

    _stmt: $ => choice(
      $.let_stmt,
      $.set_stmt,
      $.if_stmt,
      $.match_stmt,
      $.select_stmt,
      $.loop_stmt,
      $.for_stmt,
      $.while_stmt,
      $.return_stmt,
      $.give_stmt,
      $.emit_stmt,
      $.break_stmt,
      $.continue_stmt,
      $.defer_stmt,
      $.use_stmt,
      $.expr_stmt,
      $.block,
    ),

    let_stmt: $ => seq(
      choice('let', 'make', 'keep'),
      optional(choice('mut', 'move', 'ref')),
      field('name', $.pattern),
      optional(seq(':', $.type_expression)),
      '=',
      $.expression,
      optional($._semicolon)
    ),

    set_stmt: $ => seq(
      'set',
      field('target', $.expression),
      field('operator', optional(choice(
        '=',
        '+=',
        '-=',
        '*=',
        '/=',
        '%='
      ))),
      $.expression,
      optional($._semicolon)
    ),

    if_stmt: $ => seq(
      'if',
      $.expression,
      $.block,
      repeat(seq(
        'elif',
        $.expression,
        $.block
      )),
      optional(seq(
        choice('else', 'otherwise'),
        $.block
      ))
    ),

    match_stmt: $ => seq(
      'match',
      $.expression,
      '{',
      repeat($.match_arm),
      '}'
    ),

    select_stmt: $ => seq(
      'select',
      $.expression,
      $.block
    ),

    match_arm: $ => seq(
      choice('case', 'when'),
      $.pattern,
      '=>',
      choice($.expression, $.block),
      optional(',')
    ),

    loop_stmt: $ => seq(
      'loop',
      $.block
    ),

    while_stmt: $ => seq(
      choice('while', 'until'),
      $.expression,
      $.block
    ),

    for_stmt: $ => seq(
      choice('for', 'each'),
      $.pattern,
      'in',
      $.expression,
      $.block
    ),

    return_stmt: $ => seq(
      'return',
      optional($.expression),
      optional($._semicolon)
    ),

    give_stmt: $ => seq(
      'give',
      optional($.expression),
      optional($._semicolon)
    ),

    emit_stmt: $ => seq(
      'emit',
      optional($.expression),
      optional($._semicolon)
    ),

    break_stmt: $ => seq(
      'break',
      optional($._semicolon)
    ),

    continue_stmt: $ => seq(
      'continue',
      optional($._semicolon)
    ),

    defer_stmt: $ => seq(
      'defer',
      $.expression,
      optional($._semicolon)
    ),

    expr_stmt: $ => seq(
      $.expression,
      optional($._semicolon)
    ),

    //////////////////////////////////////////////////////////////////////////
    // EXPRESSIONS
    //////////////////////////////////////////////////////////////////////////

    expression: $ => choice(
      $.literal,
      $.identifier,
      $.api_identifier,
      $.diag_code,
      $.scoped_identifier,
      $.call_expr,
      $.member_expr,
      $.index_expr,
      $.binary_expr,
      $.unary_expr,
      $.assign_expr,
      $.range_expr,
      $.closure_expr,
      $.if_expr,
      $.match_expr,
      $.array_literal,
      $.tuple_literal,
      $.map_literal,
      $.block,
      $.parenthesized_expr,
    ),

    parenthesized_expr: $ => seq(
      '(',
      $.expression,
      ')'
    ),

    call_expr: $ => prec(PREC.call, seq(
      field('callee', $.expression),
      '(',
      optional(commaSep($.expression)),
      ')'
    )),

    member_expr: $ => prec(PREC.member, seq(
      $.expression,
      choice('.', '::'),
      $.identifier
    )),

    index_expr: $ => prec(PREC.member, seq(
      $.expression,
      '[',
      $.expression,
      ']'
    )),

    unary_expr: $ => prec(PREC.unary, seq(
      choice(
        'not',
        '-',
        '!',
        '*',
        '&',
        'await',
        'move',
        'borrow'
      ),
      $.expression
    )),

    binary_expr: $ => choice(

      prec.left(PREC.logic_or,
        seq($.expression, 'or', $.expression)),

      prec.left(PREC.logic_and,
        seq($.expression, 'and', $.expression)),

      prec.left(PREC.equality,
        seq($.expression,
            choice('==', '!=', 'is'),
            $.expression)),

      prec.left(PREC.compare,
        seq($.expression,
            choice('<', '>', '<=', '>='),
            $.expression)),

      prec.left(PREC.additive,
        seq($.expression,
            choice('+', '-'),
            $.expression)),

      prec.left(PREC.multiplicative,
        seq($.expression,
            choice('*', '/', '%'),
            $.expression))
    ),

    assign_expr: $ => prec.right(PREC.assign, seq(
      $.expression,
      choice(
        '=',
        '+=',
        '-=',
        '*=',
        '/=',
        '%='
      ),
      $.expression
    )),

    range_expr: $ => prec.left(PREC.range, seq(
      $.expression,
      choice('..', '..='),
      $.expression
    )),

    closure_expr: $ => seq(
      '|',
      optional(commaSep($.identifier)),
      '|',
      choice($.expression, $.block)
    ),

    if_expr: $ => seq(
      'if',
      $.expression,
      $.block,
      'else',
      choice($.expression, $.block)
    ),

    match_expr: $ => seq(
      'match',
      $.expression,
      '{',
      repeat($.match_arm),
      '}'
    ),

    //////////////////////////////////////////////////////////////////////////
    // LITERALS
    //////////////////////////////////////////////////////////////////////////

    literal: $ => choice(
      $.number_literal,
      $.float_literal,
      $.string_literal,
      $.raw_string_literal,
      $.char_literal,
      $.boolean_literal,
      $.null_literal,
    ),

    array_literal: $ => seq(
      '[',
      optional(commaSep($.expression)),
      ']'
    ),

    tuple_literal: $ => seq(
      '(',
      $.expression,
      ',',
      optional(seq($.expression, repeat(seq(',', $.expression)))),
      ')'
    ),

    map_literal: $ => seq(
      '{',
      optional(commaSep($.map_entry)),
      '}'
    ),

    map_entry: $ => seq(
      $.expression,
      ':',
      $.expression
    ),

    //////////////////////////////////////////////////////////////////////////
    // PATTERNS
    //////////////////////////////////////////////////////////////////////////

    pattern: $ => choice(
      $.identifier,
      $.scoped_identifier,
      $.tuple_pattern,
      $.wildcard_pattern,
      $.literal
    ),

    tuple_pattern: $ => seq(
      '(',
      $.pattern,
      ',',
      optional(seq($.pattern, repeat(seq(',', $.pattern)))),
      ')'
    ),

    wildcard_pattern: _ => '_',

    //////////////////////////////////////////////////////////////////////////
    // TYPES
    //////////////////////////////////////////////////////////////////////////

    type_expression: $ => choice(
      $.type_identifier,
      $.generic_type,
      $.pointer_type,
      $.array_type,
      $.tuple_type,
      $.function_type,
    ),

    generic_type: $ => seq(
      $.type_identifier,
      '<',
      commaSep1($.type_expression),
      '>'
    ),

    pointer_type: $ => seq(
      '*',
      $.type_expression
    ),

    array_type: $ => seq(
      '[',
      $.type_expression,
      ']'
    ),

    tuple_type: $ => seq(
      '(',
      $.type_expression,
      ',',
      optional(seq($.type_expression, repeat(seq(',', $.type_expression)))),
      ')'
    ),

    function_type: $ => seq(
      'proc',
      '(',
      optional(commaSep($.type_expression)),
      ')',
      optional(seq(
        '->',
        $.type_expression
      ))
    ),

    type_identifier: $ => choice(
      'void',
      'bool',
      'string',
      'str',
      'char',

      'i8',
      'i16',
      'i32',
      'i64',
      'i128',
      'isize',

      'u8',
      'u16',
      'u32',
      'u64',
      'u128',
      'usize',

      'f32',
      'f64',

      'int',
      'uint',
      'float',

      $.scoped_identifier,
      $.identifier
    ),

    //////////////////////////////////////////////////////////////////////////
    // IDENTIFIERS
    //////////////////////////////////////////////////////////////////////////

    scoped_identifier: $ => seq(
      $.identifier,
      repeat1(seq(
        choice('/', '.', '::'),
        $.identifier
      ))
    ),

    identifier: _ =>
      /[A-Za-z_][A-Za-z0-9_]*/,

    alias_pkg: _ =>
      /[A-Za-z_][A-Za-z0-9_]*_pkg/,

    api_identifier: _ =>
      /(diagnostics_|quickfix_|doctor_)[A-Za-z0-9_]*/,

    diag_code: _ =>
      /VITTE-[A-Z]+[0-9]{4}/,

    //////////////////////////////////////////////////////////////////////////
    // LITERALS
    //////////////////////////////////////////////////////////////////////////

    number_literal: _ =>
      /0x[0-9A-Fa-f_]+|0b[01_]+|0o[0-7_]+|[0-9][0-9_]*/,

    float_literal: _ =>
      /[0-9][0-9_]*\.[0-9][0-9_]*([eE][+-]?[0-9_]+)?/,

    string_literal: _ =>
      /"([^"\\]|\\.)*"/,

    raw_string_literal: _ =>
      /r"[^"]*"/,

    char_literal: _ =>
      /'([^'\\]|\\.)'/,

    boolean_literal: _ =>
      choice('true', 'false'),

    null_literal: _ =>
      choice('null', 'none'),

    //////////////////////////////////////////////////////////////////////////
    // HELPERS
    //////////////////////////////////////////////////////////////////////////

    _semicolon: _ =>
      ';',
  }
});

function commaSep(rule) {
  return optional(commaSep1(rule));
}

function commaSep1(rule) {
  return seq(rule, repeat(seq(',', rule)));
}
