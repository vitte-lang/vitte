# Checklist du build Vitte 0.1.1

Validation locale : 5 octobre 2026, macOS.

## Corrections réalisées

- [x] Passer `VERSION`, les versions publiques et le fallback des scripts à `0.1.1`.
- [x] Utiliser la version de l'API pour la CLI et les rapports SARIF.
- [x] Fournir une consigne dédiée pour chacun des 147 codes d'erreur ou d'avertissement enregistrés.
- [x] Adapter l'aide aux types attendus/reçus, au texte source, aux tokens attendus et aux contrats disponibles.
- [x] Suggérer un nom proche uniquement si une déclaration visible et sans concurrent proche existe.
- [x] Indiquer le nombre exact d'arguments manquants ou excédentaires.
- [x] Préserver l'aide explicite fournie par le sous-système.
- [x] Pour un code inconnu, conserver la cause disponible et indiquer l'absence de correction vérifiée.
- [x] Transmettre l'aide dans les rendus terminal, JSON, SARIF et LSP.
- [x] Corriger la sélection des formats : JSON et SARIF produisent leurs vrais documents.
- [x] Rendre `vitte explain CODE` accessible, en texte et en JSON, avec les mêmes consignes par code.
- [x] Actualiser les instantanés et ajouter des tests de contexte, de visibilité et de cohérence des formats.

## Validation effectuée

- [x] `make -j4 test` : suite complète réussie, dont 74 cas du corpus et 16 tests de volume.
- [x] Build CMake Release avec `-Werror` ; version du binaire : `0.1.1`.
- [x] Tests de l'aide contextuelle sur le binaire Release.
- [x] Tests de rendu, de CLI et de fuzz des diagnostics sous ASan/UBSan.
- [x] Contrôle des différences avec `git diff --check`.

## À faire pour la distribution

- [ ] Générer les installateurs et archives `0.1.1` pour les plateformes retenues.
- [ ] Vérifier installation, désinstallation et `vitte --version` sur chaque plateforme cible.
- [ ] Recalculer et vérifier les sommes SHA-256 des nouveaux artefacts.
- [ ] Publier les notes et les artefacts après validation de la matrice de distribution.

## Limites à traiter séparément

- [ ] Raccorder et tester la génération des source maps ; son test reste explicitement ignoré par la suite existante.
- [ ] Terminer la résolution sémantique des imports qualifiés homonymes ; le cas correspondant reste explicitement ignoré.

L'aide est déterministe et utilise les informations du compilateur, sans service
externe. Elle ne garantit pas une correction pour un problème inconnu. Les
suggestions orthographiques sont consultatives, limitées à un nom ASCII d'au
moins trois caractères, à une modification près, transposition comprise.

Le rendu LSP exige un convertisseur de positions configuré par l'intégration
éditeur. L'API générique retourne une erreur sans ce convertisseur, afin de ne
pas inventer des positions UTF-16.
