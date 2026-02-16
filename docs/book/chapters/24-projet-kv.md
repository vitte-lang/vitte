# 24. Projet guide KV store

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Concevoir un KV store Vitte oriente invariants en solidifiant d'abord validation des cles et protocole d'operation.

Etape 1. Poser les types de base du domaine KV.

```vit
form Entry {
  key: string
  value: string
}

pick KvResult {
  case Ok
  case ErrKey
  case ErrState
}
```

Pourquoi cette etape est solide. Les issues possibles sont bornees. Le reste du projet ne manipule plus de codes arbitraires.

Ce qui se passe a l'execution. Toutes les operations `put/get/delete` devront sortir `KvResult`.

Etape 2. Centraliser la validation de cle.

```vit
proc key_valid(k: string) -> bool {
  if k == "" { give false }
  give true
}
```

Pourquoi cette etape est solide. Une seule source de verite pour la validite syntaxique des cles.

Ce qui se passe a l'execution. `key_valid("")=false` et `key_valid("id")=true`.

Etape 3. Encadrer l'ecriture.

```vit
proc put_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len < 0 { give ErrState }
  give Ok
}
```

Pourquoi cette etape est solide. La precondition est testable sans backend concret. Le stockage reste decouple du protocole.

Ce qui se passe a l'execution. `(10,"a")->Ok`, `(10,"")->ErrKey`, `(-1,"a")->ErrState`.

Etape 4. Encadrer lecture et suppression.

```vit
proc get_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len == 0 { give ErrState }
  give Ok
}

proc delete_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len <= 0 { give ErrState }
  give Ok
}
```

Pourquoi cette etape est solide. Les trois operations partagent la meme grammaire de validation et divergent seulement sur la condition d'etat.

Ce qui se passe a l'execution. `get_guard(0,"id")=ErrState`. `delete_guard(2,"id")=Ok`.

Etape 5. Assembler une operation metier complete.

```vit
proc kv_step(entries_len: int, k: string) -> KvResult {
  let p: KvResult = put_guard(entries_len, k)
  match p {
    case Ok { give get_guard(entries_len, k) }
    case ErrKey { give ErrKey }
    otherwise { give ErrState }
  }
}
```

Pourquoi cette etape est solide. La composition montre une progression de protocole sans exposer encore la structure de persistence.

Ce qui se passe a l'execution. Avec `entries_len=3` et `k="id"`, `kv_step` retourne `Ok`. Avec `k=""`, il retourne `ErrKey`.

Ce que vous devez maitriser en sortie de chapitre. La validite de cle est unique, les transitions sont typees et le backend peut evoluer sans casser le contrat.
