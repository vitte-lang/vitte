# Vitte Stdlib Next Steps

This document tracks the broad stdlib contract batch for core slices, arrays,
comparison, hashing, allocation containers, owned strings, reference-counted
pointers, standard I/O, filesystem, environment, time, process, platform
feature detection, generated Unicode tables, and API smoke tests.

The modules are contract surfaces first. Implementations may use compiler or
backend intrinsics while preserving stable Vitte-level names and signatures.
