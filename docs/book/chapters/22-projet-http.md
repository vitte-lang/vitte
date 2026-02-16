# 22. Projet guide HTTP

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Monter un service HTTP Vitte en gardant une frontiere claire entre transport, routage metier et projection de statut.

Etape 1. Poser le modele de transport et d'erreur.

```vit
form HttpRequest {
  path: string
  body_len: int
  auth_token: string
}

pick HttpError {
  case BadRequest
  case Unauthorized
  case NotFound
}

pick HttpResult {
  case Ok(code: int)
  case Err(e: HttpError)
}
```

Pourquoi cette etape est solide. Les entites du protocole sont explicites. Les erreurs ne sont plus dispersees dans des conditions ad hoc.

Ce qui se passe a l'execution. Chaque etape du handler manipule `HttpResult`, ce qui verrouille les transitions possibles.

Etape 2. Valider la requete au bord du systeme.

```vit
proc validate_transport(r: HttpRequest) -> HttpResult {
  if r.body_len < 0 { give Err(BadRequest) }
  if r.auth_token == "" { give Err(Unauthorized) }
  give Ok(200)
}
```

Pourquoi cette etape est solide. Cette couche ne prend aucune decision metier. Elle garantit seulement que l'entree est techniquement acceptable.

Ce qui se passe a l'execution. `body_len=-1 -> Err(BadRequest)`. `auth_token="" -> Err(Unauthorized)`.

Etape 3. Isoler le routage de chemin.

```vit
proc route_path(path: string) -> HttpResult {
  if path == "/health" { give Ok(200) }
  if path == "/metrics" { give Ok(200) }
  give Err(NotFound)
}
```

Pourquoi cette etape est solide. Le routage est independant de l'authentification. Les tests peuvent cibler les chemins sans bruit annexe.

Ce qui se passe a l'execution. `/health -> Ok(200)`. `/unknown -> Err(NotFound)`.

Etape 4. Composer les etapes dans un handler.

```vit
proc handle(r: HttpRequest) -> HttpResult {
  let t: HttpResult = validate_transport(r)
  match t {
    case Err(e) { give Err(e) }
    case Ok(_) {
      give route_path(r.path)
    }
    otherwise { give Err(BadRequest) }
  }
}
```

Pourquoi cette etape est solide. La validation peut court-circuiter le routage. Le flux reste lineaire et peu imbrique.

Ce qui se passe a l'execution. Requete invalide retourne immediatement `Err`. Requete valide continue vers `route_path`.

Etape 5. Convertir le resultat en statut HTTP final.

```vit
proc to_http_code(r: HttpResult) -> int {
  match r {
    case Ok(c) { give c }
    case Err(BadRequest) { give 400 }
    case Err(Unauthorized) { give 401 }
    case Err(NotFound) { give 404 }
    otherwise { give 500 }
  }
}
```

Pourquoi cette etape est solide. La politique de code HTTP est centralisee, ce qui simplifie maintenance et audit.

Ce qui se passe a l'execution. `Err(BadRequest)->400`, `Err(Unauthorized)->401`, `Err(NotFound)->404`.

Ce que vous devez maitriser en sortie de chapitre. Le transport est filtre au bord, le routage est autonome et la projection HTTP est unique.
