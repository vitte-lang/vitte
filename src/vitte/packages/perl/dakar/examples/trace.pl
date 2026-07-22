use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Dakar qw(span_event span_finish span_start span_to_hash);

my $span = span_start(name => 'compile', start => 1);
span_event($span, 'parse');
span_finish($span, end => 2);
print span_to_hash($span)->{duration_ms} . "\n";
