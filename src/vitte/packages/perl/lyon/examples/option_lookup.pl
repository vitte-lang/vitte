use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Lyon qw(option_from_value option_map option_or);

my %config = (profile => 'release');
my $profile = option_from_value($config{profile});
my $label = option_map($profile, sub { uc $_[0] });

print option_or($label, 'DEBUG'), "\n";
