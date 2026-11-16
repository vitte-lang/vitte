# src/unicode

Category: Unicode.

Purpose: strict UTF-8 validation and Unicode XID identifier classification.

`unicode_xid_table.h` contains the pinned Unicode 16.0.0 XID_Start and
XID_Continue ranges used by the C bootstrap. It is also the input for the
native Vitte table generator: run
`python3 scripts/generate-unicode-vitte-xid.py` after deliberately changing
the checked-in C ranges. The generated Vitte module carries the same version
and uses binary search over the same ranges.

The frontend token tests cover valid XID identifiers, invalid starts,
canonically composed and decomposed spellings, invalid scalars and malformed
UTF-8. Vitte deliberately hashes source bytes and performs no implicit NFC or
NFD normalization: canonically equivalent spellings remain distinct until the
language adds an explicit normalization policy. This behavior is regression
tested so compiler hosts cannot change it accidentally.

The C implementations in `src/bootstrap/unicode/` and `src/src/unicode/`,
along with the native Vitte module, consume this table. Identifier
classification therefore stays consistent across the available frontends.
