use strict;
use warnings;
use FindBin;
use Test::More;

my $lib = "$FindBin::Bin/../lib";
my $code = 'use Vitte::City::Lyon qw(ok unwrap); die unless unwrap(ok("install")) eq "install";';
my $out = `$^X -I "$lib" -e '$code' 2>&1`;
is($?, 0, "module loads through explicit PERL5LIB path: $out");

done_testing();
