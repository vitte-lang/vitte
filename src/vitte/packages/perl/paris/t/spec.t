use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use Vitte::City::Paris qw(command_spec spec_option validate_spec usage validate_options parse_args unknown_options);

my $spec = command_spec(
    name => 'vitte',
    summary => 'toolchain',
    options => [
        spec_option(flag => '--help', description => 'show help'),
        spec_option(flag => '--output', description => 'write output'),
    ],
);

ok(validate_spec($spec)->{ok}, 'valid spec');
like(usage(%$spec), qr/--output/, 'usage includes options');
is_deeply(unknown_options(parse_args([qw(--bad --output main)]), [qw(output)]), ['bad'], 'unknown option is stable');
ok(!validate_options(parse_args([qw(--bad)]), [qw(output)])->{ok}, 'invalid parsed options');

done_testing();
