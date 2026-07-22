use strict;
use warnings;
use Test::More ();

use Vitte::City::Cairo qw(decode_record encode_record);

Test::More::ok($INC{'Vitte/City/Cairo.pm'}, 'loaded through PERL5LIB or local lib');
Test::More::is(decode_record(encode_record({ a => 1 }))->{a}, '1', 'API works after load');
Test::More::done_testing();
