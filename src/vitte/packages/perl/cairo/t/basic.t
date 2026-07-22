use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo qw(encode_scalar decode_scalar encode_record decode_record encode_list decode_list);

Test::More::is($Vitte::City::Cairo::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(decode_scalar(encode_scalar("a=b;c")), 'a=b;c', 'scalar roundtrip escapes separators');
my $record = decode_record(encode_record({ b => 2, a => 'x' }));
Test::More::is($record->{a}, 'x', 'record stores text');
Test::More::is($record->{b}, '2', 'record stores numbers as scalars');
Test::More::is_deeply(decode_list(encode_list(['a', 'b'])), ['a', 'b'], 'list roundtrip works');

Test::More::done_testing();
