use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens::Manifest qw(manifest manifest_add_dir manifest_add_file manifest_paths manifest_size);

my $m = manifest();
manifest_add_dir($m, 'bin');
manifest_add_file($m, 'bin/vitte', 1024, '0755');
manifest_add_file($m, 'share/doc.txt', 12);

Test::More::is_deeply(manifest_paths($m), ['bin', 'bin/vitte', 'share/doc.txt'], 'manifest_paths keeps order');
Test::More::is(manifest_size($m), 1036, 'manifest_size sums files only');
Test::More::is($m->{entries}->[1]->{mode}, '0755', 'file mode can be set');
Test::More::is($m->{entries}->[0]->{type}, 'dir', 'dir entry type is stable');
Test::More::done_testing();
