use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(cleanup_add cleanup_dry_run cleanup_plan);

my $plan = cleanup_plan();
cleanup_add($plan, '/tmp/athens-one', '/tmp/athens-two');
print join("\n", @{ cleanup_dry_run($plan) }), "\n";
