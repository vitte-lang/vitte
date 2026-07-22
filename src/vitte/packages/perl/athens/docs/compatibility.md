# Athens Compatibility

Athens targets Perl `5.010` and uses only core runtime facilities. Tests use
`Test::More`, `FindBin`, and `File::Temp`.

Paths are normalized with forward slashes. Windows drive prefixes are preserved
for absolute path detection and splitting.
