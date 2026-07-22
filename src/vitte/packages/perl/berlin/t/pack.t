use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin::Hex qw(to_hex);
use Vitte::City::Berlin::Pack qw(pack_i32_be pack_u16_be pack_u16_le pack_u32_be unpack_i32_be unpack_u16_be unpack_u16_le unpack_u32_be);

Test::More::is(to_hex(pack_u16_be(0x1234)), '1234', 'pack_u16_be');
Test::More::is(to_hex(pack_u16_le(0x1234)), '3412', 'pack_u16_le');
Test::More::is(unpack_u16_be(pack_u16_be(0xbeef)), 0xbeef, 'unpack_u16_be');
Test::More::is(unpack_u16_le(pack_u16_le(0xbeef)), 0xbeef, 'unpack_u16_le');
Test::More::is(to_hex(pack_u32_be(0x12345678)), '12345678', 'pack_u32_be');
Test::More::is(unpack_u32_be(pack_u32_be(0x01020304)), 0x01020304, 'unpack_u32_be');
Test::More::is(unpack_i32_be(pack_i32_be(-1)), -1, 'signed i32 roundtrip');
Test::More::done_testing();
