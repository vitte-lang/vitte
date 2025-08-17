# Signing & Verification
- Container: `cosign sign --key cosign.key ghcr.io/vitte-lang/vitte:{version}`
- Binaries: `gpg --detach-sign manifests/checksums.txt`
- Attestations: `cosign attest --predicate attestations/slsa-provenance.json --type slsaprovenance`
