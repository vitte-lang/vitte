use strict;
use warnings;
use FindBin;
use Test::More;
use JSON::PP;

open my $fh, '<', "$FindBin::Bin/../META.json" or die "open META.json: $!";
my $meta = decode_json(do { local $/; <$fh> });

is($meta->{name}, 'vitte-perl-paris', 'distribution name');
is($meta->{version}, '0.1.0', 'distribution version');
is($meta->{x_vitte_package}, 'perl/paris', 'vitte package id');
ok($meta->{provides}{'Vitte::City::Paris'}, 'main module provided');

done_testing();
