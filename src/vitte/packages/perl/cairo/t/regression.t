use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo qw(
    decode_list decode_record decode_scalar encode_list encode_record encode_scalar
    list_slice record_diff scalar_to_number table_count table_from_records table_limit
);

Test::More::is(decode_scalar(encode_scalar('')), '', 'empty scalar survives');
Test::More::is_deeply(decode_record(encode_record({ '' => '' })), { '' => '' }, 'empty record key survives');
Test::More::is_deeply(decode_list(encode_list([])), [], 'empty list survives');
Test::More::is_deeply(list_slice(['x'], 1, 0), [], 'zero-length list slice at end');
Test::More::is_deeply(record_diff({}, {}), {}, 'empty record diff');
Test::More::is(scalar_to_number('not-number', undef), undef, 'number conversion can return undef default');
Test::More::is(table_count(table_limit(table_from_records([{ a => 1 }]), 0)), 0, 'zero table limit');

Test::More::done_testing();
