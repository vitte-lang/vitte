use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Berlin qw(
    buffer from_string append length_of slice to_hex from_hex concat_buffers
    buffer_bytes byte_at clear is_empty resize reverse_bytes to_string
    buffer_clone buffer_compare buffer_equals buffer_frequency contains_bytes
    ends_with fill index_of insert_bytes pad_left pad_right remove_range
    repeat_buffer replace_range starts_with xor_buffers and_buffers or_buffers
    cursor cursor_position remaining seek read_u8 write_u8 cursor_at_end peek_u8
    read_bytes read_u16_be read_u16_le read_u32_be read_u32_le rewind skip
    write_bytes write_u16_be write_u16_le write_u32_be write_u32_le
    pack_u16_be pack_u16_le unpack_u16_be unpack_u16_le
    pack_u32_be unpack_u32_be pack_i32_be unpack_i32_be
    pack_i8 pack_i16_be pack_i16_le pack_i32_le pack_u8 pack_u32_le
    unpack_i8 unpack_i16_be unpack_i16_le unpack_i32_le unpack_u8 unpack_u32_le
    pack_bytes_be pack_bytes_le unpack_bytes_be unpack_bytes_le
    is_hex normalize_hex from_hex_strict hex_dump hex_group hex_pairs to_hex_upper
    base64_encode base64_decode base64_is_valid base64_pad base64_unpad
    base64url_decode base64url_encode
    checksum8 adler32 crc32 checksum16 crc16_ccitt fletcher16 fnv1a32 parity8 xor8
    view view_length view_slice view_to_buffer view_byte_at view_contains view_end
    view_equals view_index_of view_is_empty view_start view_to_hex
    berlin_error error_code error_context error_is error_message error_offset
    error_operation error_to_hash error_to_string error_with_context
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
Test::More::is(to_string(from_string('abc')), 'abc', 'string roundtrip');
Test::More::is(to_hex(reverse_bytes(buffer(1, 2, 3))), '030201', 'reverse_bytes reverses');

my $clone = buffer_clone($buf);
Test::More::ok(buffer_equals($buf, $clone), 'buffer_clone copies bytes');
Test::More::is(buffer_compare(buffer(1, 2), buffer(1, 3)), -1, 'buffer_compare orders lexicographically');
Test::More::is(buffer_frequency(buffer(1, 2, 1))->{1}, 2, 'buffer_frequency counts bytes');
Test::More::ok(starts_with($buf, from_string('AB')), 'starts_with detects prefix');
Test::More::ok(ends_with($buf, from_string('BC')), 'ends_with detects suffix');
Test::More::is(index_of($buf, from_string('BC')), 1, 'index_of finds byte sequence');
Test::More::ok(contains_bytes($buf, from_string('AB')), 'contains_bytes detects sequence');
insert_bytes($buf, 1, 88, 89);
Test::More::is(to_string($buf), 'AXYBC', 'insert_bytes inserts range');
Test::More::is(to_string(remove_range($buf, 1, 2)), 'XY', 'remove_range returns removed bytes');
replace_range($buf, 1, 1, 90);
Test::More::is(to_string($buf), 'AZC', 'replace_range replaces bytes');
fill($buf, 48, 1, 1);
Test::More::is(to_string($buf), 'A0C', 'fill writes byte range');
Test::More::is(to_hex(pad_left(from_string('A'), 3, 0)), '000041', 'pad_left grows at start');
Test::More::is(to_hex(pad_right(from_string('A'), 3, 0)), '410000', 'pad_right grows at end');
Test::More::is(to_string(repeat_buffer(from_string('ab'), 3)), 'ababab', 'repeat_buffer repeats');
Test::More::is(to_hex(xor_buffers(buffer(0xff, 0x0f), buffer(0x0f, 0xff))), 'f0f0', 'xor_buffers applies byte xor');
Test::More::is(to_hex(and_buffers(buffer(0xff, 0x0f), buffer(0x0f, 0xff))), '0f0f', 'and_buffers applies byte and');
Test::More::is(to_hex(or_buffers(buffer(0xf0, 0x0f), buffer(0x0f, 0xf0))), 'ffff', 'or_buffers applies byte or');
resize($buf, 5, 1);
Test::More::is(to_hex($buf), '4130430101', 'resize grows with fill');

my $cur = cursor(buffer());
write_u8($cur, 65);
write_bytes($cur, from_string('BC'));
write_u16_be($cur, 0x1234);
write_u16_le($cur, 0x5678);
write_u32_be($cur, 0x01020304);
write_u32_le($cur, 0x05060708);
Test::More::is(cursor_position($cur), length_of($cur), 'cursor advances through writes');
rewind($cur);
Test::More::is(peek_u8($cur), 65, 'peek_u8 does not advance');
Test::More::is(read_u8($cur), 65, 'read_u8 reads byte');
Test::More::is(to_string(read_bytes($cur, 2)), 'BC', 'read_bytes reads range');
Test::More::is(read_u16_be($cur), 0x1234, 'read_u16_be');
Test::More::is(read_u16_le($cur), 0x5678, 'read_u16_le');
Test::More::is(read_u32_be($cur), 0x01020304, 'read_u32_be');
Test::More::is(read_u32_le($cur), 0x05060708, 'read_u32_le');
Test::More::ok(cursor_at_end($cur), 'cursor_at_end after reads');
seek($cur, 1);
skip($cur, 2);
Test::More::is(cursor_position($cur), 3, 'seek and skip position cursor');
Test::More::ok(remaining($cur) > 0, 'remaining reports unread bytes');

Test::More::is(to_hex(pack_u8(255)), 'ff', 'pack_u8');
Test::More::is(unpack_u8(pack_u8(255)), 255, 'unpack_u8');
Test::More::is(unpack_i8(pack_i8(-1)), -1, 'i8 roundtrip');
Test::More::is(to_hex(pack_u16_be(0x1234)), '1234', 'pack_u16_be');
Test::More::is(to_hex(pack_u16_le(0x1234)), '3412', 'pack_u16_le');
Test::More::is(unpack_u16_be(pack_u16_be(0xbeef)), 0xbeef, 'unpack_u16_be');
Test::More::is(unpack_u16_le(pack_u16_le(0xbeef)), 0xbeef, 'unpack_u16_le');
Test::More::is(unpack_i16_be(pack_i16_be(-2)), -2, 'i16 be roundtrip');
Test::More::is(unpack_i16_le(pack_i16_le(-3)), -3, 'i16 le roundtrip');
Test::More::is(to_hex(pack_u32_be(0x12345678)), '12345678', 'pack_u32_be');
Test::More::is(to_hex(pack_u32_le(0x12345678)), '78563412', 'pack_u32_le');
Test::More::is(unpack_u32_be(pack_u32_be(0x01020304)), 0x01020304, 'unpack_u32_be');
Test::More::is(unpack_u32_le(pack_u32_le(0x01020304)), 0x01020304, 'unpack_u32_le');
Test::More::is(unpack_i32_be(pack_i32_be(-7)), -7, 'i32 big endian roundtrip');
Test::More::is(unpack_i32_le(pack_i32_le(-8)), -8, 'i32 little endian roundtrip');
Test::More::is_deeply(unpack_bytes_be(pack_bytes_be(1, 2, 3)), [1, 2, 3], 'pack_bytes_be');
Test::More::is_deeply(unpack_bytes_le(pack_bytes_le(1, 2, 3)), [1, 2, 3], 'pack_bytes_le');

Test::More::ok(is_hex('00 ff'), 'hex validator accepts whitespace');
Test::More::is(normalize_hex('0xAB CD'), 'abcd', 'normalize_hex strips prefix and whitespace');
Test::More::is(to_hex(from_hex_strict('00ff')), '00ff', 'from_hex_strict decodes tight hex');
Test::More::is_deeply(hex_pairs(buffer(0, 15, 255)), ['00', '0f', 'ff'], 'hex_pairs');
Test::More::is(hex_group(buffer(0, 15, 255), 2, ':'), '000f:ff', 'hex_group');
Test::More::like(hex_dump(from_string('Berlin'), 4), qr/00000000/, 'hex_dump renders offsets');
Test::More::is(to_hex_upper(buffer(0xab, 0xcd)), 'ABCD', 'to_hex_upper');

Test::More::is(to_string(base64_decode(base64_encode(from_string('Berlin')))), 'Berlin', 'base64 roundtrip');
Test::More::ok(base64_is_valid('QmVybGlu'), 'base64_is_valid accepts valid data');
Test::More::is(base64_pad('TQ'), 'TQ==', 'base64_pad adds padding');
Test::More::is(base64_unpad('TQ=='), 'TQ', 'base64_unpad removes padding');
Test::More::is(to_string(base64url_decode(base64url_encode(from_string('Berlin')))), 'Berlin', 'base64url roundtrip');

Test::More::is(checksum8(buffer(1, 2, 3)), 6, 'checksum8 sums bytes');
Test::More::is(checksum16(buffer(255, 1)), 256, 'checksum16 sums bytes');
Test::More::is(xor8(buffer(1, 2, 3)), 0, 'xor8 folds bytes');
Test::More::is(parity8(buffer(1)), 1, 'parity8 computes parity');
Test::More::is(adler32(from_string('abc')), 0x024d0127, 'adler32 known value');
Test::More::is(sprintf('%04x', fletcher16(from_string('abc'))), '4c27', 'fletcher16 known value');
Test::More::is(sprintf('%04x', crc16_ccitt(from_string('123456789'))), '29b1', 'crc16_ccitt known value');
Test::More::is(sprintf('%08x', crc32(from_string('abc'))), '352441c2', 'crc32 known value');
Test::More::is(sprintf('%08x', fnv1a32(from_string('abc'))), '1a47e90b', 'fnv1a32 known value');

my $v = view(from_string('abcdef'), 1, 3);
Test::More::is(view_start($v), 1, 'view_start');
Test::More::is(view_end($v), 4, 'view_end');
Test::More::is(view_length($v), 3, 'view_length');
Test::More::ok(!view_is_empty($v), 'view_is_empty');
Test::More::is(view_byte_at($v, 1), ord('c'), 'view_byte_at');
Test::More::is(to_string(view_to_buffer(view_slice($v, 1, 2))), 'cd', 'view slice to buffer');
Test::More::is(view_to_hex($v), '626364', 'view_to_hex');
Test::More::ok(view_contains($v, from_string('cd')), 'view_contains');
Test::More::is(view_index_of($v, from_string('cd')), 1, 'view_index_of');
Test::More::ok(view_equals($v, view(from_string('xbcdy'), 1, 3)), 'view_equals');

my $err = berlin_error('BERLIN_E_TEST', 'message', operation => 'decode', offset => 7, input => 'xx');
Test::More::is(error_code($err), 'BERLIN_E_TEST', 'error_code');
Test::More::is(error_message($err), 'message', 'error_message');
Test::More::is(error_operation($err), 'decode', 'error_operation');
Test::More::is(error_offset($err), 7, 'error_offset');
Test::More::is(error_context($err)->{input}, 'xx', 'error_context');
Test::More::ok(error_is($err, 'BERLIN_E_TEST'), 'error_is');
Test::More::like(error_to_string($err), qr/BERLIN_E_TEST/, 'error_to_string');
Test::More::is(error_to_hash(error_with_context($err, extra => 1))->{context}->{extra}, 1, 'error_with_context');

clear($buf);
Test::More::ok(is_empty($buf), 'clear empties buffer');
Test::More::done_testing();
