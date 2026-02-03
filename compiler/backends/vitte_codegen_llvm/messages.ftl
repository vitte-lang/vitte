# ============================================================
# vitte_codegen_llvm — Diagnostic messages (FTL)
# ============================================================


# ------------------------------------------------------------
# Général
# ------------------------------------------------------------

llvm-backend-name = Backend LLVM

llvm-backend-init = Initialisation du backend LLVM
llvm-backend-done = Backend LLVM initialisé avec succès


# ------------------------------------------------------------
# Erreurs générales
# ------------------------------------------------------------

llvm-error-internal =
    Erreur interne du backend LLVM.
    Détails : { $details }

llvm-error-unsupported =
    Fonctionnalité non supportée par le backend LLVM.
    Élément : { $feature }

llvm-error-invalid-state =
    État interne invalide du backend LLVM.
    Contexte : { $context }


# ------------------------------------------------------------
# Cible / Target
# ------------------------------------------------------------

llvm-error-target-not-found =
    Cible LLVM introuvable.
    Triple : { $triple }

llvm-error-target-machine-failed =
    Impossible de créer le TargetMachine LLVM.
    Triple : { $triple }
    CPU : { $cpu }
    Features : { $features }


# ------------------------------------------------------------
# Module LLVM
# ------------------------------------------------------------

llvm-error-module-verify =
    Vérification du module LLVM échouée.
    Le module généré est invalide.

llvm-info-module-verify-ok =
    Vérification du module LLVM réussie.

llvm-info-module-lowering =
    Lowering MIR → LLVM pour le module « { $module } »


# ------------------------------------------------------------
# Fonctions
# ------------------------------------------------------------

llvm-error-function-verify =
    Vérification LLVM échouée pour la fonction « { $function } ».

llvm-error-function-undeclared =
    Appel à une fonction non déclarée.
    Fonction : { $function }

llvm-info-function-lowering =
    Lowering de la fonction « { $function } »

llvm-info-function-done =
    Fonction « { $function } » générée avec succès.


# ------------------------------------------------------------
# Expressions / Instructions
# ------------------------------------------------------------

llvm-error-expr-unsupported =
    Expression MIR non supportée par le backend LLVM.
    Type : { $expr }

llvm-error-expr-null =
    Le lowering d’une expression LLVM a produit une valeur nulle.
    Expression : { $expr }


# ------------------------------------------------------------
# Types
# ------------------------------------------------------------

llvm-error-type-unsupported =
    Type Vitte non supporté par le backend LLVM.
    Type : { $type }

llvm-error-type-mismatch =
    Incohérence de types lors du lowering LLVM.
    Attendu : { $expected }
    Obtenu : { $actual }


# ------------------------------------------------------------
# Passes LLVM
# ------------------------------------------------------------

llvm-info-passes-start =
    Démarrage du pipeline de passes LLVM.
    Niveau d’optimisation : { $opt }

llvm-info-passes-done =
    Pipeline de passes LLVM terminé.

llvm-error-passes-failed =
    Une passe LLVM a échoué durant l’optimisation.


# ------------------------------------------------------------
# Émission
# ------------------------------------------------------------

llvm-info-emit-start =
    Émission LLVM vers le fichier :
    Chemin : { $path }
    Format : { $format }

llvm-info-emit-done =
    Émission LLVM terminée avec succès.
    Fichier généré : { $path }

llvm-error-emit-open-failed =
    Impossible d’ouvrir le fichier de sortie.
    Chemin : { $path }

llvm-error-emit-failed =
    Échec de l’émission LLVM.
    Format : { $format }


# ------------------------------------------------------------
# Tests / Debug
# ------------------------------------------------------------

llvm-info-smoke-start =
    Démarrage du smoke test du backend LLVM.

llvm-info-smoke-ok =
    Smoke test du backend LLVM réussi.

llvm-error-smoke-failed =
    Échec du smoke test du backend LLVM.
