use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Axum qw(route router dispatch response);

Test::More::is($Vitte::Crates::Axum::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $r=route(router(),"GET","/",sub{response(200,"ok")}); dispatch($r,"GET","/")->{status} == 200 }, 'basic behavior works');
