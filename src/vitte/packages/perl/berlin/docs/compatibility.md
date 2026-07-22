# Berlin Compatibility

Berlin targets Perl `5.010` and uses only core runtime facilities. Tests use
`Test::More` and `FindBin`.

Integer packing is implemented with shifts and byte buffers so archive behavior
does not depend on host endianness.
