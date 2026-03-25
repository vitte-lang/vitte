# Release Notes (Proof-First)

Generate proof-oriented release notes from measured artifacts:

```sh
make public-benchmark-dashboard
make release-proof-notes
```

Outputs:

- `target/reports/release_notes_proof.md`
- `target/reports/release_notes_proof.json`

Policy:

- cite only measured reports
- keep claims inside protected/public surfaces
- include tag candidate and exact evidence files
