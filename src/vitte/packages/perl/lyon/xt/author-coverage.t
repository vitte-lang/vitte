use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Lyon ();

my @exports = @Vitte::City::Lyon::EXPORT_OK;
cmp_ok(scalar(@exports), '>=', 80, 'Lyon exposes a broad public surface');

my $basic = do {
    open my $fh, '<', "$FindBin::Bin/../t/basic.t" or die $!;
    local $/;
    <$fh>;
};

for my $symbol (@exports) {
    like($basic, qr/\b\Q$symbol\E\b/, "basic.t covers export $symbol");
}

done_testing();
