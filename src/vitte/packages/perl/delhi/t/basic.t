use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Delhi qw(render_template indent dedent wrap_words slugify lines);

Test::More::is($Vitte::City::Delhi::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(render_template('hello {{ name }}', { name => 'vitte' }), 'hello vitte', 'template renders');
Test::More::is(indent("a\nb", '> '), "> a\n> b", 'indent prefixes lines');
Test::More::is(dedent("  a\n  b"), "a\nb", 'dedent trims leading whitespace');
Test::More::like(wrap_words('aa bb cc', 5), qr/\n/, 'wrap_words wraps');
Test::More::is(slugify('Hello Vitte!'), 'hello-vitte', 'slugify normalizes');

Test::More::done_testing();
