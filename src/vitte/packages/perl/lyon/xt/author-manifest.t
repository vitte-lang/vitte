use strict;
use warnings;
use FindBin;
use Test::More;

my $root = "$FindBin::Bin/..";
open my $fh, '<', "$root/MANIFEST" or die "open MANIFEST: $!";
my %listed = map { chomp; $_ => 1 } grep { /\S/ && !/^#/ } <$fh>;

my @files = sort grep { -f } map {
    my $base = $_;
    my @out;
    require File::Find;
    File::Find::find(sub { push @out, $File::Find::name if -f $File::Find::name }, $base);
    @out;
} ($root);

for my $path (@files) {
    $path =~ s/^\Q$root\E\///;
    next if $path eq 'MANIFEST';
    ok($listed{$path}, "MANIFEST lists $path");
}

done_testing();
