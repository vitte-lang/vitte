use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Paris qw(parse_args command option has_flag);

my $parsed = parse_args(\@ARGV);
print join(
    ":",
    command($parsed) || "",
    option($parsed, "output", "a.out"),
    has_flag($parsed, "verbose"),
), "\n";
