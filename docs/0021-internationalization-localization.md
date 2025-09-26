# RFC 0021 — Internationalization (i18n) and Localization (l10n)

## Résumé
Ce RFC définit le support de **l’internationalisation (i18n)** et de la **localisation (l10n)** dans le langage **Vitte** et ses outils.  
Il vise à permettre aux applications écrites en Vitte d’être facilement traduites, adaptées aux formats régionaux et accessibles globalement.

## Motivation
Dans un monde interconnecté, un langage de programmation doit faciliter :  
- la traduction des interfaces utilisateur,  
- l’adaptation aux formats locaux (dates, nombres, devises),  
- la gestion correcte des encodages et alphabets (Unicode complet),  
- la création de logiciels accessibles à un public mondial.  

## Design proposé

### Encodage par défaut
- **UTF-8** utilisé comme encodage universel.  
- Chaînes de caractères `String` et `&str` stockées en UTF-8.  
- API claire pour conversions (`utf16`, `latin1`, etc.).  

### Gestion des locales
- Module standard `std::locale`.  
- Détection automatique de la locale système.  
- Possibilité de surcharge par variable d’environnement ou configuration.  

### Traductions
- Fichiers de ressources `.po` / `.json` pris en charge.  
- API simple :  

```vitte
use std::i18n

fn main() {
    let hello = i18n::tr("hello.world")
    print(hello)
}
```

- Support du **pluralisme** :  
```json
{
  "item_count": {
    "one": "1 item",
    "other": "{} items"
  }
}
```

### Formats régionaux
- Dates (`std::time::format_localized`) respectent la locale.  
- Nombres (`1_000.5` vs `1 000,5`).  
- Devises (`USD`, `EUR`, `JPY`) avec affichage local.  

### Exemple d’utilisation
```vitte
use std::i18n
use std::time

fn main() {
    i18n::set_locale("fr_FR")
    print(i18n::tr("welcome.message"))
    print(time::now().format_localized())
}
```

### Intégration CI/CD
- Vérification automatique de traductions manquantes.  
- `vitpm lint i18n` → avertit si une clé est absente.  

### Outils complémentaires
- `vitpm i18n extract` → extrait les chaînes à traduire.  
- `vitpm i18n compile` → compile les fichiers `.po` en format binaire optimisé.  

## Alternatives considérées
- **Pas de support natif, laisser aux libs externes** : rejeté pour manque d’uniformité.  
- **Support uniquement anglais** : rejeté pour accessibilité limitée.  
- **Encodages multiples** : rejeté au profit d’UTF-8 universel.  

## Impact et compatibilité
- Impact fort sur l’accessibilité internationale des logiciels Vitte.  
- Compatible avec Unicode et formats régionaux modernes.  
- Ajoute une légère complexité à la stdlib mais bénéfice universel.  

## Références
- [Unicode Standard](https://unicode.org/)  
- [ICU (International Components for Unicode)](https://icu.unicode.org/)  
- [Gettext](https://www.gnu.org/software/gettext/)  
- [Rust i18n](https://github.com/kellpossible/cargo-i18n)  
