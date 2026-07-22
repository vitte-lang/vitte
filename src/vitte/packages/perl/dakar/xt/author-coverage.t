use strict;
use warnings;
use Test::More ();

open(my $fh, '<', 'lib/Vitte/City/Dakar.pm') or die "read main module: $!";
my $text = do { local $/; <$fh> };
my ($exports) = $text =~ /our \@EXPORT_OK = qw\((.*?)\);/s;
my @exports = grep { length } split /\s+/, $exports;

Test::More::cmp_ok(scalar(@exports), '>=', 100, 'main API has broad public surface');
for my $name (@exports) {
    Test::More::like($text, qr/\b\Q$name\E\b/, "$name appears in main module");
}

Test::More::done_testing();
