# ============================================================
# vitte_mir_transform — messages.ftl
# Diagnostics & messages du sous-système MIR transform
# ============================================================


# ------------------------------------------------------------
# Général
# ------------------------------------------------------------

mir.transform.init = Initialisation du contexte MIR transform
mir.transform.done = Transformations MIR terminées

mir.transform.debug.enabled = Mode debug MIR transform activé
mir.transform.debug.disabled = Mode debug MIR transform désactivé


# ------------------------------------------------------------
# Pipeline
# ------------------------------------------------------------

mir.transform.pipeline.start = Démarrage du pipeline MIR transform
mir.transform.pipeline.end = Fin du pipeline MIR transform

mir.transform.pipeline.pass.start =
    Démarrage de la passe MIR: { $pass }

mir.transform.pipeline.pass.end =
    Fin de la passe MIR: { $pass }

mir.transform.pipeline.pass.skip =
    Passe MIR ignorée: { $pass }


# ------------------------------------------------------------
# Simplify
# ------------------------------------------------------------

mir.transform.simplify.start = Simplification MIR démarrée
mir.transform.simplify.done = Simplification MIR terminée

mir.transform.simplify.fold =
    Constant folding appliqué dans le bloc { $block }

mir.transform.simplify.neutral =
    Opération neutre supprimée dans le bloc { $block }


# ------------------------------------------------------------
# Unreachable
# ------------------------------------------------------------

mir.transform.unreachable.start =
    Suppression des blocs inatteignables démarrée

mir.transform.unreachable.done =
    Suppression des blocs inatteignables terminée

mir.transform.unreachable.block.removed =
    Bloc inatteignable supprimé: { $block }


# ------------------------------------------------------------
# Inlining
# ------------------------------------------------------------

mir.transform.inlining.start = Inlining MIR démarré
mir.transform.inlining.done = Inlining MIR terminé

mir.transform.inlining.skip.recursive =
    Inlining ignoré (récursion): { $function }

mir.transform.inlining.skip.too_large =
    Inlining ignoré (fonction trop grande): { $function }

mir.transform.inlining.applied =
    Fonction inlinée: { $callee } dans { $caller }


# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

mir.transform.cleanup.start = Nettoyage MIR démarré
mir.transform.cleanup.done = Nettoyage MIR terminé

mir.transform.cleanup.copy.removed =
    Copie triviale supprimée dans le bloc { $block }

mir.transform.cleanup.block.removed =
    Bloc MIR vide supprimé: { $block }


# ------------------------------------------------------------
# Diagnostics
# ------------------------------------------------------------

mir.transform.warn.inlining.no_callees =
    Inlining activé mais aucune fonction appelée fournie

mir.transform.warn.non_convergent =
    La passe MIR { $pass } n’a pas convergé après { $iterations } itérations

mir.transform.error.internal =
    Erreur interne du MIR transform: { $details }


# ------------------------------------------------------------
# Debug / Dump
# ------------------------------------------------------------

mir.transform.dump.function =
    Dump MIR après transformation pour la fonction { $function }

mir.transform.dump.block =
    Bloc MIR { $block }

mir.transform.dump.instr =
    Instruction MIR: { $instr }
