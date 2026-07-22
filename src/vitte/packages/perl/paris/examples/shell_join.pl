use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Paris qw(shell_join);

print shell_join("vitte", "build", "src/main.vit", "-o", "build/main"), "\n";
