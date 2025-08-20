# Image officielle Devcontainers pour Rust
FROM mcr.microsoft.com/devcontainers/rust:1

# Paquets build/GUI/Electron optionnels + outils
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends         build-essential clang lld cmake ninja-build pkg-config         libssl-dev zlib1g-dev curl wget ca-certificates git jq         python3 python3-pip         libxkbfile-dev libsecret-1-dev libgtk-3-dev libwebkit2gtk-4.0-dev libayatana-appindicator3-dev librsvg2-dev         && rm -rf /var/lib/apt/lists/*

# Node via nvm (optionnel, mais on utilisera la feature "node" pour gérer la version)
# Rien ici : la feature installe la bonne version.
