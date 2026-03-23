# Troubleshooting

This page covers common setup and workflow failures.

## Build Fails Because `bin/vitte` Does Not Exist

Run:

```sh
make build
```

Then verify:

```sh
make quickstart-check
```

## `clang++` Cannot Find Standard C++ Headers

The repository already supports a fallback to `g++` in some local environments.

If you see missing C++ standard library headers:

- verify `clang++` is installed correctly
- verify your system C++ standard library headers are installed
- check whether the local fallback selected `g++`

## OpenSSL Or `libcurl` Build Issues

On macOS:

```sh
brew install llvm openssl@3 curl
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 make build
```

On Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install -y clang libssl-dev libcurl4-openssl-dev
make build
```

## Snapshot Or Gate Confusion

If you changed language behavior:

- run `make core-language-gate`

If you changed package/module policy:

- run `make modules-tests`
- run `make modules-snapshots`

If you changed completions:

- run `make ci-completions`

## Unsure What To Run

Start with:

```sh
make doctor
make quickstart-check
```

Then use:

- `docs/TEST_STRATEGY.md`
- `docs/CI_WORKFLOWS.md`
- `CONTRIBUTING.md`
