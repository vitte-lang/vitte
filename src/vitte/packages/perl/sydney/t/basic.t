use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Sydney qw(request response header route router dispatch status_text);

Test::More::is($Vitte::City::Sydney::VERSION, '0.1.0', 'version is 0.1.0');

my $app = router(route('GET', '/health', sub { response(200, 'ok') }));
my $res = dispatch($app, request(method => 'GET', path => '/health'));
Test::More::is($res->{body}, 'ok', 'router dispatches matching request');
Test::More::is(dispatch($app, request(path => '/missing'))->{status}, 404, 'router returns 404');
header($res, 'Content-Type', 'text/plain');
Test::More::is(header($res, 'content-type'), 'text/plain', 'headers are case-normalized');
Test::More::is(status_text(200), 'OK', 'status text resolves');

Test::More::done_testing();
