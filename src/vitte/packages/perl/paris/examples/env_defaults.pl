use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Paris qw(parse_args parse_env merge_defaults option);

my $env = parse_env("VITTE_", \%ENV);
my $parsed = merge_defaults(parse_args(\@ARGV), { profile => $env->{profile} || "debug" });
print option($parsed, "profile", "debug"), "\n";
