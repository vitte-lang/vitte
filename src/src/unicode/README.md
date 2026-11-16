# src/unicode

Category: Unicode.

Purpose: strict UTF-8 handling and Unicode XID identifier classification.

Identifier predicates use Unicode 16.0.0 XID_Start and
XID_Continue ranges from `src/bootstrap/unicode/unicode_xid_table.h`. The
version is available through `vitte_unicode_data_version()`. Vitte compares
identifier source bytes as written and does not normalize Unicode spellings.

This implementation is part of the C17 source tree configured by `src/CMakeLists.txt`.
