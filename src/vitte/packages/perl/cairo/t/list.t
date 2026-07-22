use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::List qw(
    decode_list encode_list list list_compact list_contains list_count
    list_flatten list_index_of list_join list_pop list_push list_reverse
    list_shift list_slice list_sort list_unique list_unshift
);

my $list = list('b', 'a', 'b');
list_push($list, 'c');
Test::More::is(list_pop($list), 'c', 'push/pop');
list_unshift($list, 'z');
Test::More::is(list_shift($list), 'z', 'unshift/shift');
Test::More::is_deeply(decode_list(encode_list(['a;b', 'c=d'])), ['a;b', 'c=d'], 'list codec roundtrip');
Test::More::is_deeply(list_unique($list), ['b', 'a'], 'unique keeps first');
Test::More::is_deeply(list_sort($list, 'desc'), ['b', 'b', 'a'], 'sort desc');
Test::More::is_deeply(list_reverse(['a', 'b']), ['b', 'a'], 'reverse');
Test::More::is_deeply(list_slice(['a', 'b', 'c'], 1, 1), ['b'], 'slice');
Test::More::is(list_join(['a', 'b'], '|'), 'a|b', 'join');
Test::More::ok(list_contains($list, 'a'), 'contains');
Test::More::is(list_index_of($list, 'a'), 1, 'index_of');
Test::More::is(list_count($list), 3, 'count');
Test::More::is_deeply(list_compact(['', undef, 'x']), ['x'], 'compact');
Test::More::is_deeply(list_flatten([['a'], ['b', ['c']]]), ['a', 'b', 'c'], 'flatten');

Test::More::done_testing();
