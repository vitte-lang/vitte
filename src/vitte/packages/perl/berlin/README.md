# vitte-perl-berlin

Version: `0.1.0`

Berlin is a Perl utility package for byte buffers, binary views, cursors,
integer packing, hex/base64 codecs, checksums, and structured binary errors.

## Module

`Vitte::City::Berlin`

## Public API

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
- `cursor`
- `cursor_position`
- `remaining`
- `seek`
- `read_u8`
- `write_u8`
- `pack_u16_be`
- `pack_u16_le`
- `unpack_u16_be`
- `unpack_u16_le`
- `pack_u32_be`
- `unpack_u32_be`
- `pack_i32_be`
- `unpack_i32_be`
- `is_hex`
- `base64_encode`
- `base64_decode`
- `checksum8`
- `adler32`
- `crc32`
- `view`
- `view_length`
- `view_slice`
- `view_to_buffer`
- `berlin_error`

## Submodules

- `Vitte::City::Berlin::Buffer`
- `Vitte::City::Berlin::Hex`
- `Vitte::City::Berlin::Cursor`
- `Vitte::City::Berlin::Pack`
- `Vitte::City::Berlin::Base64`
- `Vitte::City::Berlin::Checksum`
- `Vitte::City::Berlin::View`
- `Vitte::City::Berlin::Error`

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

The repository gate compiles every module, compiles examples, verifies the
machine index, builds a reproducible archive, writes a SHA256 checksum, extracts
the archive, and reruns tests through `PERL5LIB`.
