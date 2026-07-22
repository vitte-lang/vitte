use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Buffer qw(from_string buffer);
use Vitte::City::Berlin::Checksum qw(adler32 checksum8 crc32);

Test::More::is(checksum8(buffer(1, 2, 3)), 6, 'checksum8');
Test::More::is(adler32(from_string('abc')), 0x024d0127, 'adler32 known value');
Test::More::is(sprintf('%08x', crc32(from_string('abc'))), '352441c2', 'crc32 known value');
Test::More::done_testing();
