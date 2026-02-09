# 22. Projet 2 : mini‑serveur (version détaillée)

Ce projet montre comment assembler plusieurs modules pour gérer un flux I/O continu. L’idée est de rester simple : pas de magie, seulement des blocs clairs. Un mini‑serveur est un excellent exercice, car il combine lecture, parsing, réponse, et gestion des erreurs dans un flux répétitif. Ce chapitre développe le projet au maximum en paragraphes continus, sans listes, et avec un fil pédagogique clair.

## Étape 0 : cadrer l’objectif

Nous voulons un serveur qui écoute un port, lit une requête HTTP simple, puis répond avec un texte fixe. Ce n’est pas un serveur complet, c’est un exercice de structure. L’objectif est de comprendre le flux “accepter → lire → répondre → fermer”. Ce flux doit être clair, car il sera répété pour chaque connexion.

## Étape 1 : définir le flux principal

Un serveur est une boucle. Vous acceptez une connexion, vous lisez, vous répondez, vous fermez. Tout le reste est du détail. La meilleure stratégie est de coder ce flux en premier, même avec des fonctions vides, pour voir l’architecture globale.

## Étape 2 : parsing minimal

Le parsing HTTP peut être complexe, mais nous ne voulons pas cette complexité ici. Nous voulons seulement extraire la première ligne. Cela suffit pour comprendre la structure d’une requête et pour introduire une séparation claire entre lecture et logique.

## Étape 3 : réponse simple

La réponse doit être un texte fixe avec un header `Content-Length`. Ce header est une excellente occasion de rappeler que la protocolisation est un contrat. Si `Content-Length` est incorrect, le client se bloque. Ce détail montre pourquoi la précision compte, même dans un exemple simple.

## Étape 4 : erreurs et logs

Un serveur sans logs est un serveur aveugle. Mais un serveur trop bavard est un serveur qui noie l’information. La règle pragmatique est de loguer les erreurs et les événements importants, pas chaque octet lu. L’objectif est d’avoir un signal utile, pas un bruit constant.

## Étape 5 : version minimale fonctionnelle (API actuelle)

L’exemple suivant utilise `std/net/tcp`, `std/net/addr`, `std/bytes`, et `std/io/buffer`. Le code est volontairement explicite. Il privilégie la lisibilité à la sophistication.

```vit
use std/net/tcp
use std/net/addr
use std/bytes
use std/io/buffer
use std/io/print
use std/core/result.Result
use std/core/types.u16
use std/core/types.u8
use std/core/types.usize

proc parse_first_line(s: string) -> string {
  let bytes = s.as_bytes()
  let i: usize = 0
  loop {
    if i >= bytes.len { break }
    if bytes[i] == 10 { break }
    i = i + 1
  }
  give s.slice(0, i)
}

proc handle(stream: *tcp.TcpStream) {
  let buf = buffer.alloc_u8_slice(4096)
  let r = tcp.read(stream, &buf)
  when r is Result.Err {
    let _ = eprintln("read failed")
    give
  }
  let n = r.unwrap()
  let data = bytes.bytes_to_string(buf.slice(0, n))
  let _ = parse_first_line(data)
  let body = "hello from vitte\n"
  let resp = "HTTP/1.1 200 OK\r\nContent-Length: " + body.len.to_string() + "\r\n\r\n" + body
  let out = bytes.string_to_bytes(resp)
  let _ = tcp.write(stream, &out)
  let _ = tcp.close(stream)
}

entry main at core/app {
  let addr = addr.SocketAddr(
    ip = addr.IpAddr.V4(addr.IpV4(a = 127, b = 0, c = 0, d = 1)),
    port = 8080 as u16
  )
  let l = tcp.bind(addr)
  when l is Result.Err {
    let _ = eprintln("bind failed")
    give 1
  }
  let listener = l.unwrap()
  loop {
    let c = tcp.accept(&listener)
    when c is Result.Err { continue }
    let stream = c.unwrap()
    handle(&stream)
  }
  give 0
}
```

Ce code est minimal, mais il montre un flux complet. Il est suffisant pour comprendre un serveur sans se perdre dans les détails d’HTTP.

## Étape 6 : limiter la taille des requêtes

Un serveur qui lit sans limite est vulnérable. La discipline consiste à imposer un plafond de lecture. Même un serveur minimal doit définir une limite raisonnable, comme 4 KB ou 8 KB, pour éviter une consommation mémoire incontrôlée. Cette limite est une forme de sécurité.

## Étape 7 : timeouts

Un serveur sans timeout peut rester bloqué indéfiniment sur un client silencieux. La solution est simple : un timeout de lecture ou un timeout de connexion. Même si l’API est bas niveau, le concept doit être intégré dès maintenant. Un timeout est un contrat de disponibilité.

## Étape 8 : stabilité du protocole

Le protocole HTTP a des règles strictes. Même si nous ne respectons pas tout, nous devons respecter l’essentiel : status line, headers, séparation `\r\n\r\n`. C’est ce qui rend notre serveur compatible avec un client standard.

## Étape 9 : variantes utiles

Vous pouvez ajouter un endpoint `/health`, ou rendre la réponse dynamique en fonction d’un paramètre simple. Ces variantes doivent rester petites. Le but est de garder un code lisible, pas de simuler un framework.

## Étape 10 : sécurité minimale

Même un serveur minimal doit éviter certains pièges : accepter des requêtes infinies, accepter des lignes trop longues, ou répondre avec un `Content-Length` incorrect. Ces erreurs ne sont pas des détails, elles sont des bugs de protocolisation.

## Conclusion

Vous avez construit un mini‑serveur lisible et stable. Vous avez appris le flux principal, la discipline du parsing minimal, et l’importance des limites. La valeur de ce projet n’est pas dans la richesse des fonctionnalités, mais dans la clarté de l’architecture. C’est cette clarté qui vous permettra d’ajouter des fonctionnalités sans créer un monstre illisible.


## Étape 11 : routing minimal

Le routing n’a pas besoin d’un framework. Un simple `if` sur la première ligne de la requête suffit pour distinguer `/` et `/health`. La règle est de garder cette logique lisible et localisée. Si vous commencez à empiler les routes, il faut alors extraire une fonction dédiée, mais tant que le nombre de routes est faible, la clarté prime.

## Étape 12 : parsing plus robuste

Le parsing minimal lit une ligne. Un parsing un peu plus robuste doit identifier la méthode, le chemin, et la version. Cela reste simple, mais la structure doit être explicite. Le lecteur doit comprendre en une minute comment la ligne est découpée. Toute astuce de parsing qui rend le code opaque est un coût futur.

## Étape 13 : headers essentiels

Les headers HTTP ne sont pas infinis. Pour un mini‑serveur, vous pouvez en gérer deux ou trois : `Content-Length`, `Connection`, et éventuellement `Host`. L’important est de ne pas ignorer ce que vous dites. Si vous envoyez un header, il doit être correct. Un header incorrect est pire qu’une absence de header.

## Étape 14 : keep‑alive et fermeture propre

Un serveur minimal peut fermer après chaque réponse, mais un serveur plus réaliste peut accepter `Connection: keep-alive`. Cela ajoute de la complexité, car vous devez relire plusieurs requêtes sur la même connexion. La bonne stratégie est d’implémenter d’abord la fermeture propre, puis d’ajouter le keep‑alive seulement si vous en avez besoin.

## Étape 15 : limites et protection

Imposez une limite stricte sur la taille de la requête et sur la taille des headers. Même un serveur pédagogique doit éviter de se faire saturer par un client trop bavard. Ce choix est une forme de sécurité, pas une optimisation.

## Étape 16 : tests manuels simples

Testez votre serveur avec un navigateur, puis avec un client simple (curl ou netcat). Observez si la réponse est correcte, si le header `Content-Length` correspond, et si la connexion se ferme proprement. Ces tests sont rapides et donnent un signal fiable.

## Étape 17 : erreurs explicites

Les erreurs doivent être visibles. Une erreur de parsing doit produire un message clair, et un status `400`. Une erreur interne doit produire un `500`. Même si ces réponses sont minimales, elles rendent votre serveur plus professionnel.

## Étape 18 : version “presque production”

Une version presque production n’est pas un serveur complet, mais elle respecte trois règles : limites strictes, réponses correctes, et fermeture propre. Si votre mini‑serveur respecte ces règles, il devient un excellent socle pour apprendre et pour étendre sans chaos.

