# 16a. Liaison native avec `std/bridge` (version poche)

Prérequis: `docs/book/poche/SOMMAIRE.md`.
Voir aussi: `docs/book/chapters/16a-liaison-native.md`.

## Objectif

Comprendre liaison native avec `std/bridge` de manière simple pour agir rapidement.

## Idée clé en 3 points

1. Identifier le besoin concret.
2. Appliquer la forme minimale correcte.
3. Vérifier le résultat avec un test court.

## Quand l'utiliser

1. Quand vous avez besoin d'une solution lisible immédiatement.
2. Quand vous voulez valider une base avant d'aller plus loin.
3. Quand vous devez expliquer rapidement le sujet à quelqu'un d'autre.

## Exemple minimal

```vit
// Scenario liaison native: execution complete et verifiable
space demo/liaison-native

form AbiEnvelope { version: int payload_size: int flags: int }
pick NativeCall { case Ok(code: int) case Err(code: int) }

proc abi_version() -> int { give 3 }

// Validation ABI: refuse toute incompatibilité avant appel natif
proc validate_abi(e: AbiEnvelope) -> int {

  if e.version != abi_version() { give 51 }

  if e.payload_size <= 0 { give 52 }

  if e.payload_size > 4096 { give 53 }

  if e.flags < 0 { give 54 }

  give 0
}

// Appel natif simulé: exécution seulement si la règle est valide
proc call_native(e: AbiEnvelope) -> NativeCall {
  let v: int = validate_abi(e)

  if v != 0 { give NativeCall.Err(v) }

  if e.payload_size % 2 == 0 { give NativeCall.Ok(0) }

  give NativeCall.Err(55)
}

// Conversion finale vers un code de sortie
proc to_exit(r: NativeCall) -> int {

  match r {
    case Ok(c) { give c }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: AbiEnvelope = AbiEnvelope(3, 128, 1)
  let r: NativeCall = call_native(e)

  return to_exit(r)
}
```

## Erreurs fréquentes

1. Vouloir couvrir tous les cas d'un coup.
2. Mélanger plusieurs changements dans la même étape.
3. Oublier de relire le message d'erreur exact.

## Mini-exercice (5 minutes)

Modifiez l'exemple pour créer une variante de liaison native avec `std/bridge`, puis vérifiez le résultat attendu.

## Checklist rapide

1. Le code compile.
2. Le résultat est compréhensible.
3. Le cas limite principal est testé.

## Corrigé minimal

Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.

## Aller plus loin

Version complète: `docs/book/chapters/16a-liaison-native.md`.
