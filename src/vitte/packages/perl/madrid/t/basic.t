use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Madrid qw(nil_id is_nil normalize_id is_id format_id new_counter_id short_id);

Test::More::is($Vitte::City::Madrid::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::ok(is_id(nil_id()), 'nil id is valid');
Test::More::ok(is_nil(nil_id()), 'nil id detected');
my $id = new_counter_id(7);
Test::More::ok(is_id($id), 'counter id is valid');
Test::More::is(length(short_id($id)), 8, 'short id length is stable');
Test::More::is(normalize_id('00000000000040008000000000000000'), nil_id(), 'compact id normalizes');

Test::More::done_testing();
