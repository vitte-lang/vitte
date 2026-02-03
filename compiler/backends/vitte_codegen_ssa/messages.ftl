# ============================================================
# vitte_codegen_ssa — Diagnostic messages (FTL)
# ============================================================


# ------------------------------------------------------------
# Général
# ------------------------------------------------------------

ssa-backend-name =
    Backend SSA

ssa-backend-init =
    Initialisation du backend SSA.

ssa-backend-done =
    Backend SSA initialisé avec succès.


# ------------------------------------------------------------
# Erreurs internes
# ------------------------------------------------------------

ssa-error-internal =
    Erreur interne du backend SSA.
    Détails : { $details }

ssa-error-invalid-state =
    État interne invalide du backend SSA.
    Contexte : { $context }


# ------------------------------------------------------------
# Lowering MIR -> SSA
# ------------------------------------------------------------

ssa-info-lowering-start =
    Début du lowering MIR → SSA.

ssa-info-lowering-function =
    Lowering de la fonction « { $function } ».

ssa-error-lowering-unsupported-stmt =
    Instruction MIR non supportée lors du lowering SSA.
    Type : { $stmt }

ssa-error-lowering-unsupported-expr =
    Expression MIR non supportée lors du lowering SSA.
    Type : { $expr }

ssa-error-lowering-unsupported-type =
    Type MIR non supporté lors du lowering SSA.
    Type : { $type }

ssa-error-lowering-failed =
    Échec du lowering MIR → SSA.
    Fonction : { $function }


# ------------------------------------------------------------
# IR SSA
# ------------------------------------------------------------

ssa-error-invalid-module =
    Le module SSA est invalide.

ssa-error-invalid-function =
    La fonction SSA « { $function } » est invalide.

ssa-error-invalid-block =
    Le bloc SSA « { $block } » est invalide.

ssa-error-invalid-value =
    Valeur SSA invalide détectée.
    Valeur : { $value }


# ------------------------------------------------------------
# Optimisations SSA
# ------------------------------------------------------------

ssa-info-opt-start =
    Démarrage des optimisations SSA.
    Niveau : { $level }

ssa-info-opt-pass =
    Exécution de la passe SSA « { $pass } ».

ssa-info-opt-done =
    Optimisations SSA terminées avec succès.

ssa-error-opt-failed =
    Échec d’une passe d’optimisation SSA.
    Passe : { $pass }


# ------------------------------------------------------------
# CFG
# ------------------------------------------------------------

ssa-info-cfg-cleanup =
    Nettoyage du graphe de contrôle (CFG).

ssa-error-cfg-invalid =
    CFG SSA invalide détecté.
    Fonction : { $function }


# ------------------------------------------------------------
# CSE
# ------------------------------------------------------------

ssa-info-cse-start =
    Élimination des sous-expressions communes (CSE).

ssa-error-cse-invalid =
    Erreur lors de la passe CSE SSA.


# ------------------------------------------------------------
# DCE
# ------------------------------------------------------------

ssa-info-dce-start =
    Élimination du code mort (DCE).

ssa-error-dce-invalid =
    Erreur lors de la passe DCE SSA.


# ------------------------------------------------------------
# Vérification SSA
# ------------------------------------------------------------

ssa-info-verify-start =
    Démarrage de la vérification SSA.

ssa-info-verify-ok =
    Vérification SSA réussie.

ssa-error-verify-failed =
    Vérification SSA échouée.
    Élément : { $item }


# ------------------------------------------------------------
# Émission SSA
# ------------------------------------------------------------

ssa-info-emit-start =
    Émission du module SSA.
    Format : { $format }

ssa-info-emit-done =
    Émission SSA terminée avec succès.

ssa-error-emit-failed =
    Échec de l’émission SSA.
    Format : { $format }


# ------------------------------------------------------------
# Pont vers backends
# ------------------------------------------------------------

ssa-info-backend-dispatch =
    Dispatch SSA vers le backend « { $backend } ».

ssa-error-backend-unsupported =
    Backend cible non supporté par SSA.
    Backend : { $backend }

ssa-error-backend-failed =
    Échec de l’émission vers le backend « { $backend } ».


# ------------------------------------------------------------
# Debug / Tests
# ------------------------------------------------------------

ssa-info-dump =
    Dump SSA généré.

ssa-info-smoke-start =
    Démarrage du smoke test SSA.

ssa-info-smoke-ok =
    Smoke test SSA réussi.

ssa-error-smoke-failed =
    Smoke test SSA échoué.
