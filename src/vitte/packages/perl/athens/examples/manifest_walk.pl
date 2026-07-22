use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Vitte::City::Athens qw(manifest manifest_add_dir manifest_add_file manifest_paths);

my $manifest = manifest();
manifest_add_dir($manifest, 'bin');
manifest_add_file($manifest, 'bin/vitte', 4096);
print join("\n", @{ manifest_paths($manifest) }), "\n";
