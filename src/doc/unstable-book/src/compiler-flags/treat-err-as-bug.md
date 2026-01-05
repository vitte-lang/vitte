# `treat-err-as-bug`

This feature is perma-unstable and has no tracking issue.

------------------------

This flag converts the selected error to a [`bug!`] call, exiting the compiler immediately and allowing you to generate a backtrace of where the error occurred.
For full documentation, see [the c-dev-guide][dev-guide-backtrace].

Note that the compiler automatically sets `_BACKTRACE=1` for itself, and so you do not need to set it yourself when using this flag.

[`bug!`]: https://doc.-lang.org/nightly/nightly-c/c_middle/macro.bug.html
[dev-guide-backtrace]: https://c-dev-guide.-lang.org/compiler-debugging.html#getting-a-backtrace-for-errors
