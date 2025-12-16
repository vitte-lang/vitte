# NOTICE — Third‑party attributions (compiler/third_party)

Ce fichier regroupe les **attributions** requises par certaines licences (ex: Apache-2.0) pour les dépendances vendorisées dans `compiler/third_party/`.

Règles :
- Ce fichier n’est pas un inventaire exhaustif de toutes les licences (voir les `LICENSE` dans chaque dépendance).
- Ajouter ici uniquement ce qui est **explicitement requis** par l’upstream (NOTICE/attribution).
- Chaque dépendance vendorisée doit avoir :
  - `compiler/third_party/<dep>/LICENSE` (obligatoire)
  - `compiler/third_party/<dep>/UPSTREAM.md` (obligatoire)
  - `compiler/third_party/<dep>/NOTICE` si l’upstream l’exige

---

## Index

- [Format d’entrée](#format-dentrée)
- [Attributions](#attributions)

---

## Format d’entrée

Copier/coller le NOTICE upstream (ou un résumé conforme si permis) et indiquer :
- **Nom**
- **Version / commit**
- **Licence**
- **Source**
- **Chemin vendorisé**

Modèle :

```
### <dep_name>

- Version: <tag/commit>
- License: <license>
- Source: <upstream URL>
- Vendored at: compiler/third_party/<dep_name>/

<NOTICE text required by upstream>
```

---

## Attributions

> TODO: Ajouter les dépendances vendorisées qui exigent un NOTICE.
> 
> Si aucune dépendance ne requiert de NOTICE, ce fichier peut rester minimal (mais garder l’en-tête).

### Template (exemple)

- Version: <tag/commit>
- License: <license>
- Source: <upstream URL>
- Vendored at: compiler/third_party/<dep_name>/

<NOTICE text>

```
