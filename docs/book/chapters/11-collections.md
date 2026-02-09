# 11. Collections et itération

Les collections sont l’ossature des programmes réels. Vous manipulerez des listes, des maps, et des buffers, parfois très grands. L’objectif est d’être explicite sur ce que vous stockez et sur la manière dont vous parcourez ces données.

## Itération claire

La lisibilité prime. Une boucle courte et un bon nom de variable valent mieux qu’un « truc malin ». L’itération est la partie de votre code la plus lue : elle doit être simple.

## Choisir la bonne structure

Liste quand l’ordre compte. Map quand la clé compte. Buffer quand la performance compte.

## Coût des opérations

La plupart des bugs de performance viennent d’un choix implicite de structure. Une collection n’est pas neutre : elle impose un coût de lecture, d’écriture, et de mémoire.

## Slices et vues

Une bonne pratique consiste à exposer des vues (slices) plutôt que des copies. Cela réduit la mémoire et évite les surprises, tant que la durée de vie est claire.

## Itération et invariants

Ne parcourez pas une collection sans savoir ce que vous en attendez. Une boucle qui ne décrit pas son objectif est une boucle dangereuse. Ajoutez un commentaire d’intention si nécessaire.

## Erreurs courantes

Muter une collection pendant l’itération sans le dire. Utiliser une liste quand une map est requise. Copier de gros buffers par accident.

## À retenir

Le choix d’une collection est un choix d’interface. Un mauvais choix se paye en complexité.


## Exemple guidé : choisir la structure

Vous devez compter des occurrences de mots. Écrivez d’abord avec une liste, puis avec une map. Comparez la complexité.

## Checklist collections

La structure correspond à l’usage dominant. Les copies sont évitées. L’itération reste lisible.


## Exercice : éviter la copie

Créez une fonction qui reçoit une liste de 10 000 éléments. Faites une version qui copie, puis une version qui utilise une vue. Comparez la mémoire.


## Code complet (API actuelle)

Exemple : compter les occurrences d’un mot avec une map naïve (ici une liste de paires, faute de map dédiée dans la stdlib).

```vit
form Pair {
  key: string
  value: i32
}

proc inc(list: [Pair], key: string) -> [Pair] {
  let i: i32 = 0
  loop {
    if i >= list.len as i32 { break }
    if list[i as usize].key == key {
      list[i as usize].value = list[i as usize].value + 1
      give list
    }
    i = i + 1
  }
  set list = list.push(Pair(key = key, value = 1))
  give list
}
```

## API idéale (future)

On voudrait une `Map[string, i32]` standard, avec `get_or_default`, `insert`, et `keys`. L’exemple ci‑dessus deviendrait un one‑liner.

