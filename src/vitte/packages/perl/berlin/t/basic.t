use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin qw(
    buffer from_string append length_of slice to_hex from_hex concat_buffers
    buffer_bytes byte_at clear is_empty resize reverse_bytes to_string
    cursor cursor_position remaining seek read_u8 write_u8
    pack_u16_be pack_u16_le unpack_u16_be unpack_u16_le
    pack_u32_be unpack_u32_be pack_i32_be unpack_i32_be
    is_hex base64_encode base64_decode checksum8 adler32 crc32
    view view_length view_slice view_to_buffer berlin_error
);

Test::More::is($Vitte::City::Berlin::VERSION, '0.1.0', 'version is 0.1.0');
my $buf = buffer(65, 66);
append($buf, 67);
Test::More::is(length_of($buf), 3, 'length tracks appended bytes');
Test::More::is(to_hex($buf), '414243', 'hex encoding is stable');
Test::More::is(to_hex(slice($buf, 1, 2)), '4243', 'slice copies byte range');
Test::More::is(to_hex(concat_buffers(from_hex('00ff'), from_string('A'))), '00ff41', 'buffers concatenate');
Test::More::is_deeply(buffer_bytes($buf), [65, 66, 67], 'buffer_bytes copies bytes');
Test::More::is(byte_at($buf, 1), 66, 'byte_at returns indexed byte');
Test::More::ok(!is_empty($buf), 'non-empty buffer detected');
resize($buf, 5, 1);
Test::More::is(to_hex($buf), '4142430101', 'resize grows with fill');
Test::More::is(to_string(from_string('abc')), 'abc', 'string roundtrip');
Test::More::is(to_hex(reverse_bytes(buffer(1, 2, 3))), '030201', 'reverse_bytes reverses');

seek(cursor($buf), 0);
Test::More::is(read_u8($buf), 65, 'cursor reads byte');
write_u8($buf, 90);
Test::More::is(cursor_position($buf), 2, 'cursor advances after write');
Test::More::ok(remaining($buf) > 0, 'remaining reports unread bytes');

Test::More::is(unpack_u16_be(pack_u16_be(0x1234)), 0x1234, 'u16 big endian roundtrip');
Test::More::is(unpack_u16_le(pack_u16_le(0x1234)), 0x1234, 'u16 little endian roundtrip');
Test::More::is(unpack_u32_be(pack_u32_be(0x12345678)), 0x12345678, 'u32 big endian roundtrip');
Test::More::is(unpack_i32_be(pack_i32_be(-7)), -7, 'i32 big endian roundtrip');
Test::More::ok(is_hex('00 ff'), 'hex validator accepts whitespace');
Test::More::is(to_string(base64_decode(base64_encode(from_string('Berlin')))), 'Berlin', 'base64 roundtrip');
Test::More::is(checksum8(buffer(1, 2, 3)), 6, 'checksum8 sums bytes');
Test::More::ok(adler32(from_string('abc')) > 0, 'adler32 returns value');
Test::More::ok(crc32(from_string('abc')) > 0, 'crc32 returns value');
my $view = view(from_string('abcdef'), 1, 3);
Test::More::is(view_length($view), 3, 'view length');
Test::More::is(to_string(view_to_buffer(view_slice($view, 1, 2))), 'cd', 'view slice to buffer');
Test::More::is(berlin_error('BERLIN_E_TEST', 'message')->{package}, 'berlin', 'error package');
clear($buf);
Test::More::ok(is_empty($buf), 'clear empties buffer');
Test::More::done_testing();
