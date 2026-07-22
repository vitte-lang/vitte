use strict;
use warnings;
use FindBin;
use Test::More;

my $lib = "$FindBin::Bin/../lib";
my $code = 'use Vitte::City::Paris qw(parse_args command); die unless command(parse_args([qw(build)])) eq q(build);';
my $out = `$^X -I "$lib" -e '$code' 2>&1`;
is($?, 0, "module loads through explicit PERL5LIB path: $out");

done_testing();
