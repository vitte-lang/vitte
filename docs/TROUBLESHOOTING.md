# Troubleshooting

## “linker not found” (Windows)
Installer Visual Studio Build Tools (C++), redémarrer le shell.

## “permission denied” (macOS)
`xattr -dr com.apple.quarantine target/` pour lever la quarantaine si besoin.

## “failed to run custom build command”
Lisez le message `cargo:warning=…` — certaines features/outils peuvent être requis (ex: complétions CLI).
