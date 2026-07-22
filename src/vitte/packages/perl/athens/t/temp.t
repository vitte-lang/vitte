use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use File::Temp qw(tempdir);
use Test::More ();

use Vitte::City::Athens::Atomic qw(read_text);
use Vitte::City::Athens::Temp qw(temp_dir temp_file temp_name temp_path);

my $root = tempdir(CLEANUP => 1);
Test::More::like(temp_name('athens', '.tmp'), qr/^athens-\d+-\d+-\d+\.tmp$/, 'temp_name includes prefix and suffix');
Test::More::like(temp_path($root, 'athens', '.tmp'), qr{/athens-\d+-\d+-\d+\.tmp$}, 'temp_path builds inside root');

my $dir = temp_dir($root, 'athens-dir');
Test::More::ok(-d $dir, 'temp_dir creates directory');

my $file = temp_file($root, 'athens-file', '.txt', 'content');
Test::More::ok(-f $file, 'temp_file creates file');
Test::More::is(read_text($file), 'content', 'temp_file writes content');
Test::More::done_testing();
