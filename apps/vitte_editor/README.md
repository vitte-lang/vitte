# VitteEditor

VitteEditor est un prototype d'editeur graphique pour `.vit`.

Fonctions MVP:
- multi-fichiers (onglets)
- ouverture de dossier projet
- coloration syntaxique Vitte
- suggestions/autocompletion (Ctrl+Space)
- diagnostics compilation via `vitte check --diag-json`
- navigation rapide vers erreurs

Lancement local:

```bash
python3 apps/vitte_editor/vitte_editor.py
```

Ou via wrapper:

```bash
vitte-editor
```

## Notes techniques

- L'UI est en Tkinter (desktop natif simple, Debian-friendly).
- Le moteur compile/diagnostics utilise la CLI `vitte`.
- Un moteur de suggestions en Vitte est fourni dans `vitte_editor_engine.vit`
  pour evoluer vers un backend full Vitte.
