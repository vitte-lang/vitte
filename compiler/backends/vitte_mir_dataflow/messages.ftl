# ============================================================
# vitte_mir_dataflow — messages.ftl
# Diagnostics & messages du sous-système MIR dataflow
# ============================================================


# ------------------------------------------------------------
# Général
# ------------------------------------------------------------

dataflow.init = Initialisation du contexte dataflow
dataflow.done = Analyse dataflow terminée

dataflow.debug.enabled = Mode debug dataflow activé
dataflow.debug.disabled = Mode debug dataflow désactivé


# ------------------------------------------------------------
# Graphe / CFG
# ------------------------------------------------------------

dataflow.graph.build.start = Construction du graphe dataflow depuis le CFG MIR
dataflow.graph.build.done = Graphe dataflow construit avec succès

dataflow.graph.empty = Graphe dataflow vide
dataflow.graph.invalid = Graphe dataflow invalide


# ------------------------------------------------------------
# Framework dataflow
# ------------------------------------------------------------

dataflow.framework.run.start = Démarrage de l’analyse dataflow: { $analysis }
dataflow.framework.run.done = Analyse dataflow terminée: { $analysis }

dataflow.framework.iteration = Itération { $iter } pour l’analyse { $analysis }
dataflow.framework.fixed_point = Point fixe atteint pour l’analyse { $analysis }


# ------------------------------------------------------------
# Domaine
# ------------------------------------------------------------

dataflow.domain.bottom = Valeur bottom utilisée pour le domaine { $domain }
dataflow.domain.top = Valeur top utilisée pour le domaine { $domain }

dataflow.domain.join = Jointure du domaine { $domain }
dataflow.domain.equals = Comparaison d’égalité du domaine { $domain }


# ------------------------------------------------------------
# Liveness
# ------------------------------------------------------------

dataflow.liveness.start = Analyse de vivacité démarrée
dataflow.liveness.done = Analyse de vivacité terminée

dataflow.liveness.block.in = LiveIn du bloc { $block }
dataflow.liveness.block.out = LiveOut du bloc { $block }


# ------------------------------------------------------------
# Reaching Definitions
# ------------------------------------------------------------

dataflow.reaching_defs.start = Analyse Reaching Definitions démarrée
dataflow.reaching_defs.done = Analyse Reaching Definitions terminée

dataflow.reaching_defs.block.in = Définitions atteignables à l’entrée du bloc { $block }
dataflow.reaching_defs.block.out = Définitions atteignables à la sortie du bloc { $block }


# ------------------------------------------------------------
# Constant Propagation
# ------------------------------------------------------------

dataflow.const_prop.start = Analyse de propagation de constantes démarrée
dataflow.const_prop.done = Analyse de propagation de constantes terminée

dataflow.const_prop.value.const = Valeur constante détectée: { $value }
dataflow.const_prop.value.top = Valeur inconnue (Top)
dataflow.const_prop.value.bottom = Valeur impossible (Bottom)


# ------------------------------------------------------------
# Pipeline
# ------------------------------------------------------------

dataflow.pipeline.start = Démarrage du pipeline dataflow standard
dataflow.pipeline.done = Pipeline dataflow terminé

dataflow.pipeline.skip = Analyse ignorée: { $analysis }


# ------------------------------------------------------------
# Diagnostics
# ------------------------------------------------------------

dataflow.warn.non_convergent =
    L’analyse dataflow { $analysis } n’a pas convergé après { $iterations } itérations

dataflow.error.invalid_domain =
    Domaine dataflow invalide pour l’analyse { $analysis }

dataflow.error.invalid_transfer =
    Fonction de transfert invalide pour l’analyse { $analysis }

dataflow.error.internal =
    Erreur interne du moteur dataflow: { $details }


# ------------------------------------------------------------
# Debug / Dump
# ------------------------------------------------------------

dataflow.dump.header = Résultat de l’analyse dataflow: { $analysis }
dataflow.dump.in = IN[{ $node }] = { $value }
dataflow.dump.out = OUT[{ $node }] = { $value }
