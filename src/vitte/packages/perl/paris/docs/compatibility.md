# Paris Compatibility

Runtime:

- Perl 5.10 or newer.
- No XS dependency.
- No global installation required.
- Works with explicit `-I lib` and `PERL5LIB`.

Shell contract:

- Tests do not require user shell profiles.
- Examples run from the package root with `perl -I lib`.
- The Vitte integration gate can load the package through an absolute
  `PERL5LIB` path.
