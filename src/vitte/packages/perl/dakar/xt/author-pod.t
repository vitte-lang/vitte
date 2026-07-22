use strict;
use warnings;
use Test::More ();

my @modules = (
    glob('lib/Vitte/City/Dakar*.pm'),
    glob('lib/Vitte/City/Dakar/*.pm'),
);

for my $module (sort @modules) {
    open(my $fh, '<', $module) or die "read $module: $!";
    my $text = do { local $/; <$fh> };
    Test::More::like($text, qr/^package\s+Vitte::City::Dakar/m, "$module has package declaration");
}

Test::More::done_testing();
