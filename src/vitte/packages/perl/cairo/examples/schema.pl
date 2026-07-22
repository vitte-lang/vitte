use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(field schema schema_validate_record type_integer type_string);

my $schema = schema(
    field('id', type => type_integer(), required => 1),
    field('name', type => type_string(), required => 1),
);
my $report = schema_validate_record($schema, { id => 1, name => 'cairo' });
print $report->{ok} ? "ok\n" : "fail\n";
