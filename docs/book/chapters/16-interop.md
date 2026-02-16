# 16. Interop et ABI

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Concevoir une frontiere d'interoperabilite Vitte qui conserve des contrats types clairs entre code local et code externe.

Etape 1. Encoder explicitement les donnees de frontiere.

```vit
form Request {
  code: int
  payload: string
}

proc encode_code(r: Request) -> int {
  give r.code
}
```

Pourquoi cette etape est solide. Le passage frontiere est exprime sans conversion implicite. Chaque champ traversant l'ABI est visible.

Ce qui se passe a l'execution. `encode_code(Request(200,"ok"))=200`.

Etape 2. Mapper un code externe vers un type somme interne.

```vit
pick IoResult {
  case Ok(value: int)
  case Err(errno: int)
}

proc map_errno(e: int) -> IoResult {
  if e == 0 { give Ok(0) }
  give Err(e)
}
```

Pourquoi cette etape est solide. Les erreurs systeme deviennent des variantes typees, ce qui simplifie le traitement en aval.

Ce qui se passe a l'execution. `map_errno(0)=Ok(0)` et `map_errno(13)=Err(13)`.

Etape 3. Isoler le point machine `unsafe`.

```vit
proc syscall_halt() -> int {
  unsafe {
    asm("hlt")
  }
  give 0
}
```

Pourquoi cette etape est solide. La frontiere machine est concentree dans une procedure unique, ce qui facilite l'audit de securite.

Ce qui se passe a l'execution. En contexte autorise, `hlt` est execute puis la procedure retourne `0` si le flot revient.

Ce que vous devez maitriser en sortie de chapitre. Les donnees ABI sont explicites, les erreurs externes sont typees et la zone `unsafe` est strictement bornee.
