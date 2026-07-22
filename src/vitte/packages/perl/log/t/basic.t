use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Log qw(logger log_info log_warn entries);

Test::More::is($Vitte::Crates::Log::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $l=log_info(logger(),"ok"); entries($l)->[0][0] eq "INFO" }, 'basic behavior works');
