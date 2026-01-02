# runtime public headers

Public C17 API for the Vitte runtime.

Design:
- stable ABI (versioned)
- opaque handles for VM/GC
- error codes + diagnostics hooks
- optional subsystems (jit, debugger, profiler)

Include policy:
- headers must be self-contained
- no platform headers in public surface (wrapped by pal)
