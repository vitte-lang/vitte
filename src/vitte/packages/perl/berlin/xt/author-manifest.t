use strict;
use warnings;
use Test::More ();

open(my $fh, '<', 'MANIFEST') or die "read MANIFEST: $!";
my %listed = map { chomp; $_ => 1 } <$fh>;

for my $required (qw(META.json README.md OWNERS LICENSE Changes Makefile.PL cpanfile lib/Vitte/City/Berlin.pm t/basic.t)) {
    Test::More::ok($listed{$required}, "MANIFEST lists $required");
}

Test::More::done_testing();
