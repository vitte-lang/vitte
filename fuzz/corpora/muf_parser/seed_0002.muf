# seed_0002.muf — lockfile-ish fragment with comments
[lock]
version = "v1.0.0"
resolver = "registry+https://packages.vitte.dev"

[[package]]
name = "core"
version = "1.0.1"
checksum = "deadbeef"
source = "registry"

[[package]]
name = "tools"
version = "0.0.5"
features = ["cli", "http"]
# trailing table
