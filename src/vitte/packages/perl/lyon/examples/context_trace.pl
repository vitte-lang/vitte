use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Lyon qw(context context_push context_to_hash);

my $ctx = context(file => 'main.vit', target => 'native');
context_push($ctx, 'parse', line => 12);
context_push($ctx, 'typeck', symbol => 'main');

my $hash = context_to_hash($ctx);
print $hash->{values}{file}, ":", scalar(@{ $hash->{chain} }), "\n";
