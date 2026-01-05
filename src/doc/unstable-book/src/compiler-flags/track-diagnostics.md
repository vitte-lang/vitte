# `track-diagnostics`

This feature is perma-unstable and has no tracking issue.

------------------------

This flag prints the source code span in the compiler where a diagnostic was generated, respecting [`#[track_caller]`][track_caller]. Note that this may be different from the place it was emitted.
For full documentation, see [the c-dev-guide][dev-guide-track-diagnostics].

