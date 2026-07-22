use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(decode_scalar encode_scalar);

my $text = "name=cairo;version=0.1.0\n" x 100;
my $encoded = '';
for (1 .. 1000) {
    $encoded = encode_scalar($text);
    decode_scalar($encoded);
}
print length($encoded) . "\n";
