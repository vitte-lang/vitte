use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use File::Temp qw(tempdir);
use Test::More ();

use Vitte::City::Athens::Atomic qw(atomic_write read_text write_text);
use Vitte::City::Athens::Path qw(join_path);

my $root = tempdir(CLEANUP => 1);
my $file = join_path($root, 'data.txt');

Test::More::is(write_text($file, 'first'), 5, 'write_text returns byte length');
Test::More::is(read_text($file), 'first', 'read_text reads content');
Test::More::is(atomic_write($file, 'second'), 6, 'atomic_write returns byte length');
Test::More::is(read_text($file), 'second', 'atomic_write replaces content');
Test::More::done_testing();
