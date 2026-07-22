use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Trace qw(span_add_field span_duration_ms span_event span_fields span_finish span_id span_start span_to_hash trace_id);

Test::More::like(trace_id(), qr/^trace-/, 'trace id');
Test::More::like(span_id(), qr/^span-/, 'span id');
my $span = span_start(name => 'compile', start => 1);
span_add_field($span, target => 'native');
span_event($span, 'parse', ok => 1);
span_finish($span, end => 3);
Test::More::is(span_fields($span)->{target}, 'native', 'fields');
Test::More::is(span_duration_ms($span), 2000, 'duration');
Test::More::is(span_to_hash($span)->{events}->[0]->{name}, 'parse', 'hash');

Test::More::done_testing();
