# stdlib/net

Networking library for Vitte.

Scope:
- address parsing and representation
- TCP/UDP sockets (thin runtime wrappers)
- DNS resolution (stub)
- HTTP client/server (stubs, layered)
- TLS (stub)

This module is designed as a portability facade: the actual socket syscalls live under runtime/PAL.
