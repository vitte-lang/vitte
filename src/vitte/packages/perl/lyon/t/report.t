use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(
    ok err report report_add report_count report_ok report_values report_errors
    report_summary report_to_text
);

my $report = report();
Test::More::ok(report_ok($report), 'empty report is ok');
Test::More::is(report_count($report), 0, 'empty report count');

report_add($report, ok('parsed'));
report_add($report, ok('typed'));
Test::More::is_deeply(report_values($report), [qw(parsed typed)], 'report values');
Test::More::is_deeply(report_errors($report), [], 'no report errors');
Test::More::ok(report_ok($report), 'report remains ok');

report_add($report, err('LYON_E_BUILD'));
Test::More::is(report_count($report), 3, 'report count after error');
Test::More::ok(!report_ok($report), 'report detects error');
Test::More::is_deeply(report_errors($report), ['LYON_E_BUILD'], 'report errors');
Test::More::is_deeply(
    report_summary($report),
    { total => 3, ok => 2, err => 1, passed => 0 },
    'report summary'
);
Test::More::is(report_to_text($report), 'total=3 ok=2 err=1', 'report_to_text stable');

Test::More::done_testing();
