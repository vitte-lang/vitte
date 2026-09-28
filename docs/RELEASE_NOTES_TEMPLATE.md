# Release Notes Template

Use this template when publishing a Vitte release note entry in the website, changelog, or release bundle.

## Release Metadata

- Version: `X.Y.Z`
- Release date: `YYYY-MM-DD`
- Publication reference: `tag / commit / signed attestation`
- Release channel: `stable | rc | beta | nightly`

## Summary

Short description of the release and the user-visible reason it exists.

## Artifacts

- Installer package:
- Uninstaller package:
- Tarball or archive:
- Checksums:
- Signature or notarization status:

## Included Components

- `vitte`
- `vittec`
- Toolchain runtime
- Manpages
- Shell completions
- Editor assets

Add or remove entries to match the actual payload.

## Highlights

- Item 1
- Item 2
- Item 3

## Compatibility

- Minimum supported upgrade path:
- Bootstrap or seed impact:
- Platform support:
- Known incompatible changes:

Cross-check this section against `docs/release/compatibility_matrix.md`.

## Migration Notes

- Required config changes:
- Required workflow changes:
- Optional cleanup after upgrade:

## Fixes

- Compiler:
- Driver / CLI:
- Toolchain:
- Docs:
- Packaging:

## Known Issues

- Issue:
- Workaround:

## Verification Checklist

- `make release-check`
- `make release-doctor`
- package audit completed
- uninstall flow reviewed
- compatibility matrix reviewed

## Publication Checklist

- download page updated
- changelog updated
- release notes embedded in release bundle if required
- artifact names and versions verified
