# Playbook de release
1. Construire les binaires pour chaque OS/arch et remplir `dist/archives/`.
2. `bash dist/scripts/compute_checksums.sh` → met à jour `manifests/checksums.txt`.
3. Signer (`cosign`, `gpg`), générer les attestations (`attestations/`).
4. Publier via `scripts/upload_github.sh` + publier Tap/Scoop/Winget.
5. Mettre à jour les SHA dans Homebrew/Scoop/winget.
