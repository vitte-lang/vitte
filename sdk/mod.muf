# Vitte SDK Manifest

kind = "sdk"
name = "vitte.sdk"
version = "0.2.0"

# SDK Paths
sysroot = "sysroot"
share = "sysroot/share"
include = "sysroot/include"
lib = "sysroot/lib"

# Configuration
config = "config"
config_defaults = "config/defaults.toml"
config_platforms = "config/platforms.toml"
config_features = "config/features.toml"

# Documentation
docs = "docs"
docs_sdk = "docs/SDK.md"
docs_building = "docs/BUILDING.md"
docs_ffi = "docs/FFI.md"
docs_targets = "docs/TARGETS.md"
docs_abi = "docs/ABI.md"
docs_packaging = "docs/PACKAGING.md"

# Headers (ABI stable)
headers = "sysroot/include/vitte"
header_core = "sysroot/include/vitte/core.h"
header_runtime = "sysroot/include/vitte/runtime.h"
header_platform = "sysroot/include/vitte/platform.h"
header_alloc = "sysroot/include/vitte/alloc.h"

# Libraries
libs = "sysroot/lib/vitte"
pkgconfig = "sysroot/lib/pkgconfig"

# Targets (platform configurations)
targets = "sysroot/share/vitte/targets"
targets_linux_x86_64 = "sysroot/share/vitte/targets/x86_64-linux-gnu.json"
targets_macos_x86_64 = "sysroot/share/vitte/targets/x86_64-apple-darwin.json"
targets_macos_arm64 = "sysroot/share/vitte/targets/aarch64-apple-darwin.json"
targets_windows_x86_64 = "sysroot/share/vitte/targets/x86_64-pc-windows-gnu.json"
targets_wasm = "sysroot/share/vitte/targets/wasm32-unknown-unknown.json"

# Specifications
specs = "sysroot/share/vitte/specs"

# Templates
templates = "templates"
template_cli = "templates/cli"
template_lib = "templates/lib"
template_wasm = "templates/wasm"
template_plugin = "templates/plugin"

# Toolchains
toolchains = "toolchains"
toolchain_gcc = "toolchains/gcc"
toolchain_clang = "toolchains/clang"
toolchain_msvc = "toolchains/msvc"

# Description
description = """
Vitte SDK - Complete development toolkit for Vitte programming language.

Features:
- ABI-stable C headers (core.h, runtime.h, platform.h, alloc.h)
- 5+ platform target configurations (Linux, macOS, Windows, WebAssembly)
- Project templates (CLI, Library, WebAssembly, Plugin)
- Comprehensive documentation (SDK, Building, FFI, Targets, ABI, Packaging)
- Configuration system (defaults, platforms, features)
- Toolchain support (GCC, Clang, MSVC)
- Multi-platform compilation support
"""

# Metadata
license = "Apache-2.0"
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"

[dependencies]
vitte = "0.2.0"

[features]
default = ["std", "alloc"]
std = []
alloc = []
ffi = []
cross-compile = []
lto = []
simd = []
