# CI rapide (local)

Ce projet utilise `make ci-strict` comme passe complète locale:
- alignement grammaire (`grammar-check`),
- QA docs stricte (`book-qa-strict`),
- tests négatifs (`negative-tests`),
- snapshots diagnostics (`diag-snapshots`).

## Commande en une ligne

```bash
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 CURL_DIR=/opt/homebrew/opt/curl make ci-strict
```

## Variante avec export de session

```bash
export OPENSSL_DIR=/opt/homebrew/opt/openssl@3
export CURL_DIR=/opt/homebrew/opt/curl
make ci-strict
```

## Vérification rapide des dépendances

```bash
test -d /opt/homebrew/opt/openssl@3 && echo "openssl ok"
test -d /opt/homebrew/opt/curl && echo "curl ok"
```
