use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;
use JSON::PP;

use Vitte::City::Lyon ();

open my $fh, '<', "$FindBin::Bin/../META.json" or die "open META.json: $!";
my $meta = decode_json(do { local $/; <$fh> });

is($meta->{name}, 'vitte-perl-lyon', 'distribution name');
is($meta->{version}, '0.1.0', 'distribution version');
is($meta->{x_vitte_package}, 'perl/lyon', 'vitte package id');
is_deeply(
    [ sort keys %{ $meta->{provides} } ],
    [qw(
        Vitte::City::Lyon
        Vitte::City::Lyon::Context
        Vitte::City::Lyon::Error
        Vitte::City::Lyon::Option
        Vitte::City::Lyon::Recovery
        Vitte::City::Lyon::Report
        Vitte::City::Lyon::Result
    )],
    'all modules are declared'
);

done_testing();
