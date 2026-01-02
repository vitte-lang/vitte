muf 1
package "vitte.std.net"
  version 0.1.0
  license "MIT"
.end

target "net"
  kind "vitte-lib"
  src "src/stdlib/src/net"
.end

deps
  dep "vitte.std.core" version "0.1.0"
  dep "vitte.std.io"   version "0.1.0"
.end
