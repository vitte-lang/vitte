# `compiler::util::util`

This module contains compiler utilities implemented entirely in Vitte. Checked
results replace C output pointers; string helpers operate on UTF-8 bytes.

The port currently covers checked arithmetic, alignment, ASCII classification,
FNV hashing, integer parsing, and string comparison and search. It passes the
bootstrap's semantic `check` command. The remaining C utility surface includes
allocation, byte buffers, vectors, endian reads and writes, sorting, and
binary search. Those operations need Vitte collection support before this
module can replace the whole C utility library.
