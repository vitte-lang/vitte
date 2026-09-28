# Backend

Le pipeline de production abaisse le HIR et le MIR vers l'IR backend canonique.
Les adaptateurs C, LLVM et assembleur natif consomment ce contrat commun. Les
erreurs des outils externes sont converties en diagnostics structures.

Le backend C produit une source (`--emit-c`), un objet (`--emit-obj`) ou un
executable lie (sortie par defaut de `build`).

## Comparaison C/LLVM

`backend-differential-gate` couvre un corpus versionne de six programmes :
constantes, arithmetique imbriquee, appel de procedure, branche conditionnelle,
comparaison et division signee. Chaque source est d'abord acceptee par la
commande utilisateur `vitte check`. Le profil `--strict`, reserve au corpus
self-hosting du compilateur, n'est pas applique aux programmes du corpus.

Le test de contrat
`test_c_and_llvm_backends_consume_shared_ir_corpus` envoie la meme `IrUnit`
canonique aux adaptateurs de production C et LLVM, puis verifie leur validite,
leur nombre de fonctions et les operations attendues. Le bridge de transition valide
actuellement cette suite sans executer son corps.

En complement, le gate materiel genere des artefacts C et LLVM de reference a
partir du meme arbre structure du corpus, les compile en `O0` et `O2`, les
execute, puis compare code retour, sortie standard et erreur standard. Cette
separation reste explicite dans le champ `execution_model` du rapport
`target/reports/backend_differential.json`; elle ne presente pas les artefacts
de reference comme des sorties des adaptateurs de production.
