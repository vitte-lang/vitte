use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Buffer qw(from_string to_string);
use Vitte::City::Berlin::View qw(view view_length view_slice view_to_buffer);

my $view = view(from_string('abcdef'), 1, 4);
Test::More::is(view_length($view), 4, 'view length');
Test::More::is(to_string(view_to_buffer($view)), 'bcde', 'view_to_buffer');
Test::More::is(to_string(view_to_buffer(view_slice($view, 1, 2))), 'cd', 'view_slice');
Test::More::done_testing();
