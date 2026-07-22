# vitte-perl-berlin

Version: `0.1.0`

Berlin is the Vitte Perl package for binary project tooling: byte buffers, cursor reads and writes, integer packing, hex/base64 codecs, checksums, bounded views, and structured binary errors.

## Module

`Vitte::City::Berlin`

## Public API

### Buffers

Create, clone, mutate, compare, search, pad, repeat, and combine byte buffers.

- `buffer`
- `from_string`
- `append`
- `length_of`
- `slice`
- `to_hex`
- `from_hex`
- `concat_buffers`
- `buffer_bytes`
- `byte_at`
- `clear`
- `is_empty`
- `resize`
- `reverse_bytes`
- `to_string`
- `buffer_clone`
- `buffer_compare`
- `buffer_equals`
- `buffer_frequency`
- `contains_bytes`
- `ends_with`
- `fill`
- `index_of`
- `insert_bytes`
- `pad_left`
- `pad_right`
- `remove_range`
- `repeat_buffer`
- `replace_range`
- `starts_with`
- `xor_buffers`
- `and_buffers`
- `or_buffers`

### Cursors

Read and write byte streams through a mutable cursor with bounds checks and endian helpers.

- `cursor`
- `cursor_position`
- `remaining`
- `seek`
- `read_u8`
- `write_u8`
- `cursor_at_end`
- `peek_u8`
- `read_bytes`
- `read_u16_be`
- `read_u16_le`
- `read_u32_be`
- `read_u32_le`
- `rewind`
- `skip`
- `write_bytes`
- `write_u16_be`
- `write_u16_le`
- `write_u32_be`
- `write_u32_le`

### Packing

Pack and unpack signed and unsigned integer values plus raw byte vectors in stable endian order.

- `pack_u16_be`
- `pack_u16_le`
- `unpack_u16_be`
- `unpack_u16_le`
- `pack_u32_be`
- `unpack_u32_be`
- `pack_i32_be`
- `unpack_i32_be`
- `pack_i8`
- `pack_i16_be`
- `pack_i16_le`
- `pack_i32_le`
- `pack_u8`
- `pack_u32_le`
- `unpack_i8`
- `unpack_i16_be`
- `unpack_i16_le`
- `unpack_i32_le`
- `unpack_u8`
- `unpack_u32_le`
- `pack_bytes_be`
- `pack_bytes_le`
- `unpack_bytes_be`
- `unpack_bytes_le`

### Hex

Normalize, validate, decode, encode, group, pair, uppercase, and dump hexadecimal data.

- `is_hex`
- `normalize_hex`
- `from_hex_strict`
- `hex_dump`
- `hex_group`
- `hex_pairs`
- `to_hex_upper`
- `base64_encode`
- `base64_decode`

### Base64

Encode, decode, validate, pad, unpad, and use URL-safe base64 for binary payloads.

- `base64_is_valid`
- `base64_pad`
- `base64_unpad`
- `base64url_decode`
- `base64url_encode`
- `checksum8`
- `adler32`

### Checksums

Compute compact checksums and common integrity hashes for buffers.

- `crc32`
- `checksum16`
- `crc16_ccitt`
- `fletcher16`
- `fnv1a32`
- `parity8`
- `xor8`
- `view`
- `view_length`

### Views

Create bounded binary views, slice them, compare them, search inside them, and materialize buffers.

- `view_slice`
- `view_to_buffer`
- `view_byte_at`
- `view_contains`
- `view_end`
- `view_equals`
- `view_index_of`
- `view_is_empty`
- `view_start`
- `view_to_hex`
- `berlin_error`
- `error_code`

### Structured Errors

Create and inspect stable Berlin errors for codecs, packing, cursors, and binary parsing.

- `error_context`
- `error_is`
- `error_message`
- `error_offset`
- `error_operation`
- `error_to_hash`
- `error_to_string`
- `error_with_context`

## Submodules

- `Vitte::City::Berlin::Buffer`
- `Vitte::City::Berlin::Hex`
- `Vitte::City::Berlin::Cursor`
- `Vitte::City::Berlin::Pack`
- `Vitte::City::Berlin::Base64`
- `Vitte::City::Berlin::Checksum`
- `Vitte::City::Berlin::View`
- `Vitte::City::Berlin::Error`

## Guarantees

- Versioned distribution metadata stays fixed at `0.1.0`.
- Every public export is exercised by `t/basic.t`.
- All modules compile independently under `PERL5LIB`.
- Binary operations keep bytes masked to `0..255`.
- Cursor and view operations validate bounds before reading or writing.
- Integer packing covers signed and unsigned 8/16/32-bit values in stable endian order.

## Checks

```sh
perl -Ilib t/00-load.t
perl -Ilib t/basic.t
perl -Ilib t/buffer.t
perl -Ilib t/hex.t
perl -Ilib t/cursor.t
perl -Ilib t/pack.t
perl -Ilib t/base64.t
perl -Ilib t/checksum.t
perl -Ilib t/view.t
perl -Ilib t/errors.t
perl -Ilib t/install-perl5lib.t
perl -Ilib t/regression.t
```
