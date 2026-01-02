muf 1
package "vitte.std.io"
  version 0.1.0
  license "MIT"
.end

target "io"
  kind "vitte-lib"
  src "src/stdlib/src/io"
.end

deps
  dep "vitte.std.core" version "0.1.0"
.end
