use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Oslo qw(duration seconds millis add_duration compare_duration format_duration epoch_parts);

Test::More::is($Vitte::City::Oslo::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(format_duration(add_duration(seconds(2), millis(25))), '2.025s', 'duration adds and formats');
Test::More::is(compare_duration(seconds(1), seconds(2)), -1, 'duration comparison orders values');
my $parts = epoch_parts(0);
Test::More::is($parts->{year}, 1970, 'epoch year is stable');
Test::More::is($parts->{month}, 1, 'epoch month is stable');

Test::More::done_testing();
