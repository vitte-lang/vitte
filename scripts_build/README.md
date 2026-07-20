# scripts_build

Installer builders for Vitte release artifacts.

Common options:

- `--dry-run`: print the planned builder action and exit before staging or writing artifacts.
- `--list-targets`: list supported installer target tuples from `build-all-installers.sh`.
- `--print-env`: print effective build environment from `build-all-installers.sh`.
- `--verify-only OUT_DIR`: verify an existing artifact directory.
- `--clean`: remove `target/installer-*`; requires `CONFIRM_CLEAN=YES`.

Common environment:

- `VERSION`: package version. Defaults to `toolchain/scripts/package/PACKAGE_VERSION`.
- `OUT_DIR`: output directory for artifacts. Defaults to `pkgout`.
- `ARCH`: target architecture or `all`, depending on the builder.
- `STRICT_PROCESSOR`: when `1`, payload staging refuses host fallback binaries for mismatched architectures.
- `STRICT_NATIVE`: when `1`, `verify-installers.sh` requires native Windows and Solaris packages, not only portable kits.
- `STRICT_DMG`: when `1`, macOS installer builds fail if `hdiutil` cannot create the DMG. Release jobs should set this.
- `FAMILY`: `linux`, `freebsd`, `bsd`, `macos`, `solaris`, `windows`, or `all`.
- `SIGN`: when `1`, sign macOS/Windows artifacts using platform-specific tools.
- `NOTARIZE`: when `1`, submit signed macOS artifacts to Apple notary service.
- `SBOM`: when `1`, generate SPDX and CycloneDX SBOM files.

Examples:

- Linux: `FAMILY=linux ARCH=amd64 scripts_build/build-all-installers.sh`
- BSD portable: `FAMILY=bsd BSD_FAMILY=openbsd ARCH=amd64 scripts_build/build-all-installers.sh`
- macOS release: `FAMILY=macos STRICT_DMG=1 SIGN=1 NOTARIZE=1 scripts_build/build-all-installers.sh`
- Solaris: `FAMILY=solaris ARCH=i386 scripts_build/build-all-installers.sh`
- Windows retrocompatibility kits: `FAMILY=windows ARCH=all scripts_build/build-all-installers.sh`
- Windows professional matrix: `pwsh scripts_build/windows-build.ps1 -Arch all -WindowsVersion all`

Exit codes:

- `0`: requested operation completed.
- `1`: validation/build failure.
- `2`: invalid command-line usage in target listing helpers.

Release macOS builds should run with `STRICT_DMG=1` so a missing DMG is not silently accepted.
