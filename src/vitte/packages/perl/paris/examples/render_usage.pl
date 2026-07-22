use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";

use Vitte::City::Paris qw(usage);

print usage(
    name => "vitte",
    summary => "compiler command",
    options => [
        { flag => "--help", description => "show help" },
        { flag => "--output", description => "write output" },
    ],
);
