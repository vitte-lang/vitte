# Backend du compilateur

Le pipeline de production abaisse le HIR et le MIR vers l'IR backend canonique.
Les adaptateurs C, LLVM et assembleur natif consomment ce contrat commun. Les
erreurs des outils externes sont converties en diagnostics structurés.

Le backend C produit une source (`--emit-c`), un objet (`--emit-obj`) ou un
exécutable lié (sortie par défaut de `build`).

## Comparaison C/LLVM

`backend-differential-gate` couvre un corpus versionné de six programmes :
constantes, arithmétique imbriquée, appel de procédure, branche conditionnelle,
comparaison et division signée. Chaque source est d’abord acceptée par la
commande utilisateur `vitte check`. Le profil `--strict`, réservé au corpus
self-hosting du compilateur, n’est pas appliqué aux programmes du corpus.

Le test de contrat
`test_c_and_llvm_backends_consume_shared_ir_corpus` envoie la même `IrUnit`
canonique aux adaptateurs de production C et LLVM, puis vérifie leur validité,
leur nombre de fonctions et les opérations attendues. Le pont de transition valide
actuellement cette suite sans exécuter son corps.

En complément, le gate matériel génère des artefacts C et LLVM de référence à
partir du même arbre structuré du corpus, les compile en `O0` et `O2`, les
exécute, puis compare le code de retour, la sortie standard et l’erreur standard. Cette
séparation reste explicite dans le champ `execution_model` du rapport
`target/reports/backend_differential.json`; elle ne présente pas les artefacts
de référence comme des sorties des adaptateurs de production.
