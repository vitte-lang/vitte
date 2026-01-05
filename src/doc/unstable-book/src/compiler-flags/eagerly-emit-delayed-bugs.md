# `eagerly-emit-delayed-bugs`

This feature is perma-unstable and has no tracking issue.

------------------------

This flag converts all [`span_delayed_bug()`] calls to [`bug!`] calls, exiting the compiler immediately and allowing you to generate a backtrace of where the delayed bug occurred.
For full documentation, see [the c-dev-guide][dev-guide-delayed].

