# Workspace MUF — required
workspace
  name "vitte"
  version "0.2.0"
.end

toolchain
  hermetic true
  backend "c"
  cc "zig cc"
  cflags "-std=c17 -O2"
.end

package
  name "steel"
  kind "tool"
  src "src/cli/steel_main.c"
.end

package
  name "steelc"
  kind "tool"
  src "src/cli/steelc_main.c"
.end

package
  name "std"
  kind "lib"
  src "std/src/std/prelude.vitte"
.end
