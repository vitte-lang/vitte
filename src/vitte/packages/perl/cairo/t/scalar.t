use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Scalar qw(
    decode_scalar encode_scalar escape_scalar scalar_compare scalar_default
    scalar_is_bool scalar_is_integer scalar_is_null scalar_is_number scalar_lower
    scalar_to_bool scalar_to_number scalar_to_string scalar_trim scalar_type
    scalar_upper unescape_scalar
);

Test::More::is(decode_scalar(encode_scalar("a=b;c|d\r\n\t")), "a=b;c|d\r\n\t", 'scalar codec preserves separators');
Test::More::is(unescape_scalar(escape_scalar('x=y')), 'x=y', 'escape aliases codec');
Test::More::is(scalar_type(undef), 'null', 'null type');
Test::More::ok(scalar_is_null(undef), 'null predicate');
Test::More::ok(scalar_is_bool('false'), 'bool predicate');
Test::More::ok(scalar_is_integer('+42'), 'integer predicate');
Test::More::ok(scalar_is_number('.5'), 'number predicate');
Test::More::is(scalar_to_bool('no'), 0, 'bool conversion');
Test::More::is(scalar_to_number('bad', 9), 9, 'number default');
Test::More::is(scalar_to_string(undef), '', 'string conversion');
Test::More::is(scalar_trim("  value\n"), 'value', 'trim');
Test::More::is(scalar_lower('ABC'), 'abc', 'lower');
Test::More::is(scalar_upper('abc'), 'ABC', 'upper');
Test::More::is(scalar_default('', 'x'), 'x', 'default');
Test::More::is(scalar_compare('a', 'a'), 0, 'compare equal');

Test::More::done_testing();
