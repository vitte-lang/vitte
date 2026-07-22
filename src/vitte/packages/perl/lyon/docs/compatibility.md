# Lyon Compatibility

Runtime:

- Perl 5.10 or newer.
- No XS dependency.
- No global installation required for tests.
- Works with explicit `-I lib` and `PERL5LIB`.

Shell contract:

- Tests do not require user shell profiles.
- Examples run from the package root with `perl -I lib`.
- The package can be archived reproducibly by the Vitte Perl package gate.

Data contract:

- Result and option shapes are plain hashes.
- Context key order is sorted.
- Report text format is stable: `total=N ok=N err=N`.
