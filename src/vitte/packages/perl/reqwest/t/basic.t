use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Reqwest qw(request get post header);

Test::More::is($Vitte::Crates::Reqwest::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $r=get("https://example.invalid"); $r->{method} eq "GET" }, 'basic behavior works');
