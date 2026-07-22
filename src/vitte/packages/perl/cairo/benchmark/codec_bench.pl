use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(decode_document encode_document stable_hash);

my $doc = { rows => [ map { { id => $_, name => "row$_" } } 1 .. 100 ] };
my $encoded = '';
for (1 .. 100) {
    $encoded = encode_document($doc);
    decode_document($encoded);
    stable_hash($doc);
}
print length($encoded) . "\n";
