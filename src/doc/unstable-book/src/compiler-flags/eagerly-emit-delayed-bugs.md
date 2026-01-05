# `eagerly-emit-delayed-bugs`

This feature is perma-unstable and has no tracking issue.

------------------------

This flag converts all [`span_delayed_bug()`] calls to [`bug!`] calls, exiting the compiler immediately and allowing you to generate a backtrace of where the delayed bug occurred.
For full documentation, see [the c-dev-guide][dev-guide-delayed].

[`bug!`]: https://doc.-lang.org/nightly/nightly-c/c_middle/macro.bug.html
[`span_delayed_bug()`]: https://doc.-lang.org/nightly/nightly-c/c_errors/struct.DiagCtxtHandle.html#method.span_delayed_bug
[dev-guide-delayed]: https://c-dev-guide.-lang.org/compiler-debugging.html#debugging-delayed-bugs
