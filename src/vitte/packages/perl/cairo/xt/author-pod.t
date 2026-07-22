use strict;
use warnings;
use Test::More ();

my @modules = (
    glob('lib/Vitte/City/Cairo*.pm'),
    glob('lib/Vitte/City/Cairo/*.pm'),
);

for my $module (sort @modules) {
    open(my $fh, '<', $module) or die "read $module: $!";
    my $text = do { local $/; <$fh> };
    Test::More::like($text, qr/^package\s+Vitte::City::Cairo/m, "$module has package declaration");
}

Test::More::done_testing();
