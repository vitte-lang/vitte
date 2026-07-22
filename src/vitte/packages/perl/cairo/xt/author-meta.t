use strict;
use warnings;
use JSON::PP ();
use Test::More ();

open(my $fh, '<', 'META.json') or die "read META.json: $!";
my $meta = JSON::PP->new->decode(do { local $/; <$fh> });

Test::More::is($meta->{name}, 'vitte-perl-cairo', 'distribution name');
Test::More::is($meta->{version}, '0.1.0', 'distribution version');
Test::More::ok($meta->{provides}{'Vitte::City::Cairo'}, 'main module is provided');

Test::More::done_testing();
