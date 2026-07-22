use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris ();

my @exports = @Vitte::City::Paris::EXPORT_OK;
cmp_ok(scalar(@exports), '>=', 30, 'Paris exports broad CLI surface');

my $basic = do {
    open my $fh, '<', "$FindBin::Bin/../t/basic.t" or die $!;
    local $/;
    <$fh>;
};

for my $symbol (@exports) {
    like($basic, qr/\b\Q$symbol\E\b/, "basic.t mentions export $symbol");
}

done_testing();
