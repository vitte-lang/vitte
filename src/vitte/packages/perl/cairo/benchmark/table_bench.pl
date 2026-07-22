use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(table_count table_from_records table_sort table_where);

my $table = table_from_records([ map { { id => $_, group => $_ % 3, name => "row$_" } } 1 .. 200 ]);
for (1 .. 100) {
    my $filtered = table_where($table, sub { $_[0]->{group} == 1 });
    table_sort($filtered, 'name');
}
print table_count($table) . "\n";
