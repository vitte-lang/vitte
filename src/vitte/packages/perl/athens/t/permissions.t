use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use File::Temp qw(tempdir);
use Test::More ();

use Vitte::City::Athens::Atomic qw(write_text);
use Vitte::City::Athens::Path qw(join_path);
use Vitte::City::Athens::Permissions qw(ensure_dir is_readable is_writable permissions);

my $root = tempdir(CLEANUP => 1);
my $dir = join_path($root, 'a', 'b');
ensure_dir($dir);
Test::More::ok(-d $dir, 'ensure_dir creates nested directory');

my $file = join_path($dir, 'file.txt');
write_text($file, 'data');
Test::More::ok(is_readable($file), 'file is readable');
Test::More::ok(is_writable($file), 'file is writable');
Test::More::like(permissions($file), qr/^[0-7]{4}$/, 'permissions returns octal mode');
Test::More::done_testing();
