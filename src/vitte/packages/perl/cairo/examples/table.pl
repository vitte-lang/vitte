use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(table_from_records table_join_text table_sort);

my $table = table_from_records([{ name => 'b' }, { name => 'a' }]);
print table_join_text(table_sort($table, 'name'), 'name', ',') . "\n";
