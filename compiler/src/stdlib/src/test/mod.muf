muf 1
package "vitte.std.test"
  version 0.1.0
  license "MIT"
.end

target "test"
  kind "vitte-lib"
  src "src/stdlib/src/test"
.end

deps
  dep "vitte.std.core" version "0.1.0"
  dep "vitte.std.io"   version "0.1.0"
.end
