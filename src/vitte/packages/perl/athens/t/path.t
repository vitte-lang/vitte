use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Athens::Path qw(
    join_path normalize_path basename dirname extension without_extension
    replace_extension is_absolute is_relative split_path path_components
);

Test::More::is(join_path('/root/', '/pkg', './file.txt'), '/root/pkg/file.txt', 'join_path joins and normalizes');
Test::More::is(normalize_path('a/b/../c'), 'a/c', 'relative path normalization');
Test::More::is(basename('/root/pkg/file.txt'), 'file.txt', 'basename');
Test::More::is(dirname('/root/pkg/file.txt'), '/root/pkg', 'dirname');
Test::More::is(extension('/root/pkg/file.txt'), 'txt', 'extension');
Test::More::is(without_extension('/root/pkg/file.txt'), '/root/pkg/file', 'without_extension');
Test::More::is(replace_extension('/root/pkg/file.txt', '.md'), '/root/pkg/file.md', 'replace_extension strips leading dot');
Test::More::ok(is_absolute('/root'), 'is_absolute unix');
Test::More::ok(is_absolute('C:/root'), 'is_absolute drive');
Test::More::ok(is_relative('root/file'), 'is_relative');
Test::More::is_deeply(split_path('/root/pkg'), ['root', 'pkg'], 'split_path');
Test::More::is_deeply(path_components('/root/pkg'), ['root', 'pkg'], 'path_components');
Test::More::done_testing();
