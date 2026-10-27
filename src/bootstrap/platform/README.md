# Bootstrap platform boundary

`platform.c` is the host boundary for operations used by the command driver:
current directory lookup, executable probing, path-list separators and child
process execution. POSIX `posix_spawnp`/`waitpid` and Windows wide spawn APIs
are selected here so compiler emitters do not depend on a host process API or
the active Windows code page. UTF-8 command arguments are converted at this
boundary.
Filesystem path operations accept both `/` and `\\`, drive-letter absolute
paths and UTF-8 path bytes. Directory suggestions use the filesystem
abstraction, so the same enumeration API is used on every host.

Run `make -C src portability` to inspect the remaining POSIX surface and probe
the installed C17 compilers. `make -C src portability PORTABILITY_STRICT=1`
can be used in a Windows CI job with a Windows-target Clang installation.
