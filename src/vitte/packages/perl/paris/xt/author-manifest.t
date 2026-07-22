use strict;
use warnings;
use FindBin;
use Test::More;

my $root = "$FindBin::Bin/..";
for my $required (qw(META.json README.md OWNERS LICENSE Changes Makefile.PL cpanfile lib/Vitte/City/Paris.pm t/basic.t)) {
    ok(-e "$root/$required", "$required exists");
}

open my $fh, '<', "$root/MANIFEST" or die "open MANIFEST: $!";
my %listed = map { chomp; $_ => 1 } grep { /\S/ && !/^#/ } <$fh>;
ok($listed{'lib/Vitte/City/Paris.pm'}, 'module listed in MANIFEST');
ok($listed{'t/basic.t'}, 'basic test listed in MANIFEST');

done_testing();
