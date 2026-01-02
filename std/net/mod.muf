# /Users/vincent/Documents/Github/vitte/std/net/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/net
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.net as a standalone package (workspace-member compatible).
# - Expose networking façade:
#     * address types (IpAddr, SocketAddr)
#     * TCP/UDP sockets (blocking first; async integration optional)
#     * DNS resolve helpers (optional)
#     * HTTP primitives kept OUT of std.net (belongs in std.http)
# - Depend on std-sys for OS sockets; keep test fakes where possible.
# - Keep deps minimal: std-core, std-runtime, std-string; std-io optional.
# - Include unit tests (parse/format of addresses, feature probes).
# - Feature-gate platform backends and DNS.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - For embedded/no-sys builds, disable sys_backend.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-net"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: networking facade (ip/socket tcp/udp, optional dns)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "net", "network", "tcp", "udp", "socket", "dns"]
.end

workspace
  kind = "member"
.end

target
  name = "std-net"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.net"
  .end
  entry
    module = "std.net.ip"
  .end
  entry
    module = "std.net.addr"
  .end
  entry
    module = "std.net.tcp"
  .end
  entry
    module = "std.net.udp"
  .end
  entry
    module = "std.net.dns"
  .end
.end

deps
  dep
    name = "std-core"
    path = "../core"
  .end

  dep
    name = "std-runtime"
    path = "../runtime"
  .end

  dep
    name = "std-string"
    path = "../string"
  .end

  dep
    name     = "std-io"
    path     = "../io"
    optional = true
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "ip"
    description = "Enable IP address types and parsers (IPv4/IPv6)."
    default = true
  .end

  feature
    name = "tcp"
    description = "Enable TCP sockets facade."
    default = true
  .end

  feature
    name = "udp"
    description = "Enable UDP sockets facade."
    default = true
  .end

  feature
    name = "dns"
    description = "Enable DNS resolve helpers (requires sys backend)."
    default = false
    requires = ["sys_backend"]
  .end

  feature
    name = "sys_backend"
    description = "Use OS socket backend via std-sys."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "no_alloc"
    description = "Prefer caller-provided buffers; reduce allocations."
    default = false
  .end

  feature
    name = "tests_fake_backend"
    description = "Enable deterministic fake socket backend for unit tests."
    default = true
  .end

  feature
    name = "async_integration"
    description = "Enable optional integration hooks with std-async."
    default = false
  .end
.end

profile
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-net"
  .end

  script
    name = "check"
    run  = "muffin check std-net"
  .end
.end
