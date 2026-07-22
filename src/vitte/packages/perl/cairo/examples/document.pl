use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Cairo qw(encode_document stable_hash);

my $document = { name => 'cairo', version => '0.1.0' };
print encode_document($document) . "\n";
print stable_hash($document) . "\n";
