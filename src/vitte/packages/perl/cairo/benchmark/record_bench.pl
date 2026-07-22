use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(decode_record encode_record);

my %pairs;
for my $index (1 .. 100) {
    $pairs{"k$index"} = "v$index";
}
my $record = \%pairs;
my $encoded = '';
for (1 .. 500) {
    $encoded = encode_record($record);
    decode_record($encoded);
}
print length($encoded) . "\n";
