# Commandes et déclarations avancées

## Commandes de haut niveau
Les mots-clés `program`, `software`, `kernel`, `service`, `driver`, `utility`
définissent des points d’entrée. Exemple :

```vitte
program telemetry:
    import runtime.fs
    task producer:
        spawn collect()
```

Chaque commande est pensée pour être reliée à une cible de build (exécutable,
service, utilitaire système).

## Déclarations avancées
Mots-clés : `scenario`, `capsule`, `bridge`, `device`, `blueprint`, `realm`,
`tool`, `command`, `route`, `pipeline`.

```vitte
pipeline ingest(data):
    task stage:
        await normalize(data)
```

Ces constructions permettent de structurer de gros systèmes tout en restant
lisibles pour les lycéens : arguments facultatifs, blocs à indentation claire.
