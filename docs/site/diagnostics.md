# Diagnostics Vitte

Consultation rapide des diagnostics: un code, une couche, une correction.

## Navigation

- [Accueil](/site/index.md)
- [Documentation](/site/doc.md)
- [Téléchargement](/site/download.md)
- [Source](/site/source.md)
- [Communauté](/site/community.md)
- [Actualités](/site/news.md)
- [Diagnostics](/site/diagnostics.md)
- [Suggestions](/site/suggestions.md)

## Source de vérité

Commence par le premier code, puis lis la fiche stable correspondante dans
`docs/book/errors.md`. Pour confirmer le sens du message, passe par
`docs/book/cli.md` et la commande `vitte explain <code>`.

## Séquence officielle

- `vitte explain <code>`
- `vitte check --diag-code-only`
- `vitte mod doctor --fix`
- `vitte check --diag-json-pretty`

## Codes fréquents

- [E0001 - identifiant attendu](/book/errors.md#e0001)
- [E0013 - identifiant inconnu](/book/errors.md#e0013)
- [E0019 - proc nécessite un corps](/book/errors.md#e0019)
- [E0024 - select nécessite au moins une branche when](/book/errors.md#e0024)

## Références

- [CLI et options de diagnostic](/book/cli.md)
- [Catalogue complet des diagnostics](/book/errors.md)
- [Lire une erreur en 3 étapes](/book/poche/07-lire-les-erreurs.md)
- [Générateur d’index diagnostics](/tools/build_diagnostics_index.py)

## Que faire en premier

- Lire le premier code, pas toute la cascade.
- Ouvrir la fiche correspondante dans `docs/book/errors.md`.
- Lancer `vitte explain <code>` pour confirmer le sens officiel.
- Corriger localement, puis relancer sur un cas minimal.

## Si c'est un import

- Vérifier le chemin canonique et le nom exporté.
- Vérifier l’alias explicite demandé par le contrat d'import.
- Passer par `docs/book/compiler-stdlib-contract.md` si la règle vient de la stdlib ou de la liaison native.
- Lancer `vitte mod doctor --fix` pour obtenir une proposition de réécriture.

## Si c'est une erreur de syntaxe

- Regarder d'abord les fiches [E0001](/book/errors.md#e0001), [E0002](/book/errors.md#e0002), [E0003](/book/errors.md#e0003), [E0004](/book/errors.md#e0004), [E0005](/book/errors.md#e0005).
- Vérifier les blocs non fermés, les identifiants manquants, et les expressions incomplètes.
- Corriger le premier point cassé, puis relancer avant de toucher aux erreurs dérivées.

## Si c'est un bug du compilateur

- Regarder les fiches [E0022](/book/errors.md#e0022), [E0023](/book/errors.md#e0023), [E0026](/book/errors.md#e0026), [E0027](/book/errors.md#e0027), [E0028](/book/errors.md#e0028).
- Réduire au plus petit exemple qui reproduit encore le problème.
- Conserver le code exact, la couche touchée, et la sortie de `vitte check --diag-code-only`.
- Si le bug survit à la réduction, le signaler comme anomalie de lowering ou de backend selon la couche indiquée.

## Index rapide

- **parse** : [E0001](/book/errors.md#e0001), [E0002](/book/errors.md#e0002), [E0003](/book/errors.md#e0003), [E0004](/book/errors.md#e0004), [E0005](/book/errors.md#e0005), [E0006](/book/errors.md#e0006), [E0007](/book/errors.md#e0007), [E0008](/book/errors.md#e0008)
- **resolve** : [E0009](/book/errors.md#e0009), [E0010](/book/errors.md#e0010), [E0011](/book/errors.md#e0011), [E0012](/book/errors.md#e0012), [E0013](/book/errors.md#e0013), [E0018](/book/errors.md#e0018), [E0019](/book/errors.md#e0019), [E0020](/book/errors.md#e0020), [E0021](/book/errors.md#e0021), [E0024](/book/errors.md#e0024), [E0025](/book/errors.md#e0025)
- **IR** : [E0015](/book/errors.md#e0015), [E0016](/book/errors.md#e0016), [E0017](/book/errors.md#e0017), [E0022](/book/errors.md#e0022), [E0023](/book/errors.md#e0023), [E0026](/book/errors.md#e0026), [E0027](/book/errors.md#e0027), [E0028](/book/errors.md#e0028)
