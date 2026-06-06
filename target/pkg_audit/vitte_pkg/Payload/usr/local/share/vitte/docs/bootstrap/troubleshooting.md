# Troubleshooting

Common failures:
- stage1 executable missing -> check stage0 build logs
- checksum mismatch -> run `make seed-manifest-update` then `make seed-verify`
- strict self-check fails on clang -> install clang or use non-strict gate for local baseline
