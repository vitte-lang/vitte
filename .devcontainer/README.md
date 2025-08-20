# .devcontainer — Vitte (ultra complet)

Devcontainer prêt pour développer tout l'écosystème **Vitte** : Rust toolchain, Node (IDE), WebAssembly,
fuzzing, benchmarks — avec extensions VS Code, scripts d'init et ports forwardés.

## Utilisation
- Ouvrir le repo dans VS Code → **Reopen in Container**.
- Ou en CLI :
  ```bash
  devcontainer up --workspace-folder .
  ```

## Dossiers importants
- `Dockerfile` — image base (devcontainers Rust) + paquets build essentiels.
- `devcontainer.json` — features (Rust, Node), extensions, scripts post-create.
- `scripts/` — `postCreate.sh`, `postStart.sh`, `onAttach.sh`.
- `templates/launch.code-workspace` — exemple de config launch/debug.
