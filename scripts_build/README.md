# scripts_build

Installer builders for Vitte release artifacts.

Common options:

- `--dry-run`: print the planned builder action and exit before staging or writing artifacts.

Common environment:

- `VERSION`: package version. Defaults to `toolchain/scripts/package/PACKAGE_VERSION`.
- `OUT_DIR`: output directory for artifacts. Defaults to `pkgout`.
- `ARCH`: target architecture or `all`, depending on the builder.
- `STRICT_PROCESSOR`: when `1`, payload staging refuses host fallback binaries for mismatched architectures.
- `STRICT_NATIVE`: when `1`, `verify-installers.sh` requires native Windows and Solaris packages, not only portable kits.
- `STRICT_DMG`: when `1`, macOS installer builds fail if `hdiutil` cannot create the DMG. Release jobs should set this.

Release macOS builds should run with `STRICT_DMG=1` so a missing DMG is not silently accepted.
