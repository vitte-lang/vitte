use strict;
use warnings;
use Test::More ();

for my $file (glob 'lib/Vitte/City/Berlin*.pm lib/Vitte/City/Berlin/*.pm') {
    open(my $fh, '<', $file) or die "read $file: $!";
    my $text = do { local $/; <$fh> };
    Test::More::like($text, qr/package Vitte::City::Berlin/, "$file has package declaration");
}

Test::More::done_testing();
