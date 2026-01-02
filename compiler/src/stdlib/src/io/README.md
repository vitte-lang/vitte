# stdlib/io

I/O primitives for Vitte.

Focus:
- buffering
- reader/writer traits (protocols)
- byte/char streams
- file-like abstraction (backed by runtime/sys)
- encoding helpers (utf8 baseline)

Submodules:
- traits/: reader/writer interfaces (baseline)
- buf/: buffered reader/writer
- fs/: file API wrappers (thin)
- net/: sockets (stubs)
- ser/: serialization helpers (json stub integration)
- term/: terminal io adapters (integration point with cli)

Note:
- Low-level syscalls are delegated to `runtime/lib/sys` and PAL.
