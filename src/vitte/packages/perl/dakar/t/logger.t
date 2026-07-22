use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Logger qw(clear_entries debug entries entry_count error info last_entry logger logger_clone logger_field logger_fields logger_has_field logger_level logger_name logger_restore logger_snapshot reset_logger set_capacity set_level set_name warn with_field with_fields without_field);

my $log = logger('info', name => 'compiler');
Test::More::is(logger_level($log), 'info', 'level');
Test::More::is(logger_name($log), 'compiler', 'name');
set_level($log, 'debug');
set_name($log, 'builder');
with_field($log, component => 'cli');
with_fields($log, target => 'native');
Test::More::is(logger_field($log, 'target'), 'native', 'field');
Test::More::ok(logger_has_field($log, 'component'), 'has field');
without_field($log, 'component');
Test::More::ok(!logger_has_field($log, 'component'), 'removed field');
Test::More::is(logger_fields($log)->{target}, 'native', 'fields copy');
set_capacity($log, 2);
debug($log, 'one');
info($log, 'two');
warn($log, 'three');
error($log, 'four');
Test::More::is(entry_count($log), 2, 'capacity');
Test::More::is(last_entry($log)->{message}, 'four', 'last');
Test::More::is((entries($log))[0]->{message}, 'three', 'entries');
my $snapshot = logger_snapshot($log);
clear_entries($log);
logger_restore($log, $snapshot);
Test::More::is(logger_clone($log)->{entries}->[0]->{message}, 'three', 'clone');
reset_logger($log);
Test::More::is(entry_count($log), 0, 'reset entries');

Test::More::done_testing();
