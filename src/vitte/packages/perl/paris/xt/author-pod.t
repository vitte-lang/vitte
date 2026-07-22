use strict;
use warnings;
use FindBin;
use Test::More;

for my $doc (qw(api.md design.md error-codes.md compatibility.md)) {
    my $path = "$FindBin::Bin/../docs/$doc";
    ok(-s $path, "$doc exists and is non-empty");
}

done_testing();
