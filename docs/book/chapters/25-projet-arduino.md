# 25. Projet 5 : embarqué Arduino (étape par étape)

Le monde embarqué impose des contraintes fortes. C’est exactement le terrain où la clarté du code fait la différence. Un microcontrôleur n’a pas la marge d’un serveur, et il ne pardonne pas les abstractions inutiles. Ce chapitre est un guide détaillé, étape par étape, en gros paragraphes continus, pour construire un programme Arduino minimal mais fiable, puis pour l’étendre sans perdre la lisibilité. L’objectif n’est pas de “faire beaucoup”, mais de faire juste, propre, et reproductible.

## Étape 0 : état d’esprit

Commencez par accepter trois réalités. La mémoire est limitée, le CPU est lent, et le diagnostic est difficile. Cela signifie que chaque ligne doit avoir une raison d’exister, et que chaque ajout doit être justifié par un bénéfice clair. Le code embarqué est un code de discipline. Cette discipline n’est pas une contrainte “morale”, c’est la condition qui rend le projet maintenable.

## Étape 1 : cahier des charges minimal

Nous allons écrire un programme qui fait clignoter une LED. Ce cahier des charges est volontairement modeste, mais il contient tout ce qu’il faut pour apprendre : initialiser un périphérique, appliquer un timing stable, et conserver un flux lisible. Un cahier des charges clair est le meilleur test de lisibilité. Si vous n’arrivez pas à décrire le programme en une phrase, vous ne pourrez pas le maintenir.

## Étape 2 : structure minimale du fichier

Un projet embarqué doit commencer par une structure simple. Un seul fichier, quelques imports explicites, des constantes visibles, des fonctions courtes, puis l’entrée. Cette structure évite de “l’architecture prématurée”. Vous ne gagnez rien à disperser le code si vous n’avez pas encore un besoin réel de séparation. En revanche, vous gagnez beaucoup à garder un ordre lisible.

## Étape 3 : premiers imports et constantes

La première décision concrète est d’importer les modules Arduino nécessaires. Nous utilisons le module GPIO et le module timer. Ensuite, nous définissons une constante pour le pin de la LED. Cette constante rend la configuration visible et réduit les erreurs d’édition.

```vit
use std/arduino/gpio
use std/arduino/timer
use std/core/types.u8

const LED: u8 = 13
```

L’important ici n’est pas la syntaxe, mais l’intention. Une constante `LED` explicite est plus lisible qu’un `13` perdu au milieu d’un flux.

## Étape 4 : entrée minimale

Nous écrivons l’entrée la plus simple possible. Elle configure la pin en sortie, puis alterne état haut et bas avec un délai. Ce flux est la base de presque tous les programmes embarqués : initialiser, puis boucler.

```vit
entry main at core/app {
  gpio.pin_mode(LED, gpio.PinMode.Output)
  loop {
    gpio.digital_write(LED, gpio.PinState.High)
    timer.delay_ms(500)
    gpio.digital_write(LED, gpio.PinState.Low)
    timer.delay_ms(500)
  }
  give 0
}
```

Ce code est volontairement explicite. Chaque action est visible. Vous savez exactement ce que fait le programme, et dans quel ordre. Ce niveau de clarté est un objectif, pas un hasard.

## Étape 5 : stabiliser le timing

Un clignotement stable est un test de fiabilité. Si vous observez des variations, votre boucle fait probablement plus que ce que vous pensez. La correction est rarement une optimisation : c’est une simplification. Vérifiez que votre boucle n’a qu’une seule responsabilité. Évitez d’ajouter du code “au hasard” dans la boucle principale. Chaque ajout doit être justifié.

## Étape 6 : paramétrer sans compliquer

La prochaine étape consiste à rendre la durée configurable. La règle est simple : un paramètre, une constante. Vous pouvez introduire `DELAY_MS` et remplacer les `500` par cette constante. Le gain est minime en code, mais majeur en lisibilité. L’utilisateur et le futur mainteneur voient immédiatement où modifier le comportement.

## Étape 7 : factoriser l’action

Quand le flux devient répétitif, vous pouvez factoriser. Mais factoriser n’est pas toujours un gain en embarqué. Le bon moment est celui où la factorisation rend la logique plus claire, pas celui où elle rend le code plus “joli”. Une fonction `blink_once(delay_ms)` peut être utile si elle clarifie le flux. Si elle ajoute une couche sans gain, évitez‑la.

## Étape 8 : gérer l’état explicitement

Si vous ajoutez un bouton, un capteur, ou un mode de clignotement alternatif, vous introduisez un état. Cet état doit être explicite, visible, et stable. Un `pick Mode { Slow, Fast }` est souvent plus clair que deux booléens. Cette étape est cruciale, car elle marque le passage d’un flux linéaire à un flux conditionnel.

## Étape 9 : éviter les allocations inutiles

L’embarqué ne tolère pas les allocations dynamiques inutiles. Si vous n’avez pas besoin d’un buffer dynamique, n’en créez pas. Les allocations augmentent la fragmentation et rendent le comportement plus difficile à prévoir. Cette discipline n’est pas “exagérée”, elle est essentielle.

## Étape 10 : version opérationnelle

À ce stade, votre programme est fonctionnel et lisible. Une version opérationnelle ne signifie pas “feature‑complete”, elle signifie “comportement stable et explicable”. Si vous pouvez expliquer le flux sans regarder le code, vous avez réussi.

## Étape 11 : variantes contrôlées

Une variante utile consiste à rendre le clignotement configurable par un input (bouton). Une autre variante consiste à introduire un mode “diagnostic” avec un pattern de LED différent. Ces variantes doivent être ajoutées une par une, avec un changement minimal à chaque étape. L’objectif est de conserver un système lisible malgré l’évolution.

## Étape 12 : erreurs fréquentes et corrections

Les erreurs les plus fréquentes en embarqué sont des erreurs de structure. Trop d’abstraction, pas assez de visibilité. D’autres erreurs classiques : oublier de configurer une pin, mélanger la logique de timing et de lecture d’entrée, ou disperser les décisions dans plusieurs fonctions. La correction est toujours la même : rendre le flux explicite, réduire les niveaux d’indirection, et centraliser les décisions.

## Conclusion

Ce projet n’est pas un gadget. Il vous apprend une discipline : écrire moins, mais écrire mieux. L’embarqué impose des contraintes que beaucoup de développeurs ignorent. Ces contraintes peuvent devenir des forces, si vous acceptez la simplicité comme un outil de qualité. Un programme embarqué n’a pas besoin d’être complexe pour être sérieux. Il a besoin d’être clair, stable, et explicable. C’est exactement ce que vous avez construit étape par étape.


## Étape 13 : capteurs, I2C, SPI, gestion d’énergie

À partir du moment où vous connectez un capteur, vous quittez le monde du simple GPIO. Vous entrez dans un monde de protocoles et de timing. Cela ne signifie pas “complexité gratuite”, cela signifie “discipline renforcée”. La bonne approche est de traiter chaque protocole comme un module séparé, avec des responsabilités claires et un flux lisible.

### Capteurs et lecture robuste

Un capteur n’est jamais stable. Il renvoie parfois des valeurs hors plage, parfois des erreurs. Le code embarqué doit donc traiter la lecture comme une opération potentiellement invalide. La règle simple est d’introduire un contrôle après chaque lecture, même si vous ne le faites pas encore en profondeur. L’objectif est de préparer le terrain à la robustesse.

### I2C : protocole partagé, discipline nécessaire

I2C est un bus partagé. Cela signifie que plusieurs périphériques communiquent sur les mêmes lignes. La logique doit donc être claire : initialiser le bus, sélectionner un périphérique, envoyer une commande, lire une réponse. Une lecture I2C est un mini‑contrat. Si vous oubliez une étape, vous obtenez un comportement silencieux et frustrant.

Voici un exemple avec un capteur BH1750, disponible dans la stdlib Arduino. Il montre une séquence simple : initialiser le bus, créer le capteur, puis lire une valeur. Ce code est volontairement explicite.

```vit
use std/arduino/i2c
use std/arduino/i2c/bh1750
use std/arduino/timer

entry main at core/app {
  i2c.begin()
  let dev = bh1750.new_default()
  bh1750.power_on(dev)
  loop {
    let lux = bh1750.read_lux(dev)
    timer.delay_ms(1000)
  }
  give 0
}
```

Même si l’exemple ne gère pas toutes les erreurs, il donne la structure correcte : init, config, loop de lecture. L’étape suivante consiste à ajouter des contrôles d’erreur et un filtrage simple des valeurs.

### SPI : vitesse et discipline

SPI est plus rapide que I2C, mais il exige une gestion stricte du chip select. Le risque principal est d’envoyer une séquence de bytes sans cadrer correctement le périphérique. La solution est d’encapsuler la séquence SPI dans une fonction dédiée, et d’éviter de disperser les appels dans le code.

```vit
use std/arduino/spi

proc read_device() -> u8 {
  spi.begin()
  let v = spi.transfer(0x00 as u8)
  spi.end()
  give v
}
```

Cet exemple est minimal, mais il illustre la discipline : ouverture, transfert, fermeture. Dans un projet réel, vous ajouteriez un contrôle de vitesse, un timing, et une gestion d’erreur.

### Gestion d’énergie : ressource invisible

Sur une carte embarquée, l’énergie est une ressource. Si vous voulez un programme fiable sur batterie, vous devez réduire les cycles actifs. La règle est de minimiser les boucles actives et d’utiliser des délais ou des modes basse consommation quand c’est possible. Même sans API complète de power management, vous pouvez réduire la consommation en diminuant la fréquence de lecture et en évitant les calculs inutiles.

Une discipline simple est d’introduire un cycle “mesure → attente”. Par exemple, lire un capteur, traiter, puis dormir. Cette structure se comprend immédiatement et rend l’énergie visible dans le flux du programme.

### Conclusion de l’étape

Capteurs, I2C, SPI et gestion d’énergie sont des sujets différents, mais ils partagent une même règle : l’explicite est votre allié. Si vous rendez les séquences de communication visibles et séparées, votre code devient robuste. Si vous les mélangez, vous créez un système opaque. L’embarqué ne pardonne pas l’opacité.


## Étape 14 : pipeline complet “capteur → filtre → affichage”

Cette étape construit une chaîne complète, du capteur jusqu’à une sortie visible. Même si l’embarqué n’a pas toujours de console, il a souvent un canal d’affichage minimal, comme un port série. L’objectif est de montrer une structure complète : lecture, filtrage, puis émission d’un résultat. Cette discipline est la même quel que soit le capteur.

Nous allons lire un capteur I2C, appliquer un filtre très simple, puis envoyer la valeur via le port série. Le filtre est volontairement basique : une moyenne glissante sur quelques mesures. Le but n’est pas de faire des mathématiques avancées, mais d’illustrer la structure.

```vit
use std/arduino/i2c
use std/arduino/i2c/bh1750
use std/arduino/timer
use std/arduino/serial
use std/core/types.u32

proc avg(a: u32, b: u32, c: u32) -> u32 {
  give (a + b + c) / 3
}

entry main at core/app {
  i2c.begin()
  serial.begin(9600)
  let dev = bh1750.new_default()
  bh1750.power_on(dev)

  let v0: u32 = 0
  let v1: u32 = 0
  let v2: u32 = 0

  loop {
    v0 = v1
    v1 = v2
    v2 = bh1750.read_lux(dev) as u32
    let filtered = avg(v0, v1, v2)
    // Dans un projet réel, vous convertiriez en string avant d’écrire
    let _ = serial.write((filtered & 0xFF) as u8)
    timer.delay_ms(500)
  }
  give 0
}
```

Cet exemple montre une pipeline complète. Il reste volontairement simple, mais la structure est celle d’un projet réel : acquisition, filtrage, émission. Le filtrage empêche les valeurs erratiques de dominer, et la sortie série donne un feedback minimal mais utile.

## Étape 15 : debug embarqué sans console

Le debug embarqué est une discipline particulière, parce que vous n’avez pas la console confortable d’un OS. Cela ne signifie pas “pas de debug”, mais “debug par signes”. Les signes sont des LEDs, des timings, des séquences, et parfois un port série minimal.

La première technique est le clignotement codé. Vous encodez des états avec des patterns de LED. Par exemple, un clignotement lent signifie “ok”, un clignotement rapide signifie “erreur I2C”. Ce n’est pas élégant, mais c’est très fiable.

La seconde technique est le “heartbeat”. Un clignotement stable, régulier, vous dit que le firmware tourne encore. Si le heartbeat s’arrête, vous savez immédiatement qu’une erreur bloquante s’est produite.

La troisième technique est l’utilisation du port série, même en mode minimal. Vous pouvez envoyer des octets bruts qui servent de signaux. Par exemple, envoyer `0x01` pour “capteur ok”, `0x02` pour “capteur absent”. Cette approche est primitive, mais elle donne un canal de diagnostic concret.

Enfin, la meilleure méthode reste la réduction du problème. Quand un bug apparaît, supprimez tout ce qui n’est pas nécessaire, gardez une seule lecture de capteur, et vérifiez que la chaîne fonctionne. Cette discipline de réduction est la clé du debug embarqué.


## Étape 16 : gestion des erreurs I2C/SPI

Les bus I2C et SPI sont fiables, mais pas parfaits. Les erreurs existent : périphérique absent, bus saturé, données incohérentes. En embarqué, le silence est dangereux. La meilleure stratégie est d’ajouter un traitement d’erreur minimal et visible, même si vous ne faites pas encore de récupération complexe.

Pour I2C, la règle est simple : chaque lecture doit pouvoir échouer, et cette possibilité doit être reflétée dans le flux. Si l’API ne renvoie pas explicitement une erreur, vous pouvez introduire un contrôle par valeurs plausibles. Par exemple, si un capteur renvoie une valeur hors plage, vous la rejetez et vous gardez la dernière valeur valide. Ce n’est pas parfait, mais c’est déjà une barrière contre les comportements erratiques.

Pour SPI, la discipline se situe surtout dans la séquence. Commencez, transférez, terminez. Si vous mélangez des transferts sans séparation, vous créez des erreurs difficiles à diagnostiquer. La correction n’est pas un “hack”, c’est un retour à une séquence claire.

Une approche simple consiste à définir des codes d’état, puis à refléter ces codes par un signal visuel. Par exemple, une LED qui clignote deux fois signifie “erreur I2C”, trois fois signifie “erreur SPI”. Ce type de signal n’est pas élégant, mais il est extrêmement utile quand vous n’avez pas de console.

## Étape 17 : calibration des capteurs

Un capteur brut n’est pas un capteur utile. La calibration consiste à ajuster les valeurs brutes pour qu’elles correspondent à une réalité physique. Le principe est simple : mesurer un point de référence, calculer un facteur, appliquer ce facteur à toutes les mesures suivantes. Cette logique peut être aussi simple que deux constantes, mais elle doit être explicite dans le code.

Une calibration minimale comprend un “offset” et un “scale”. L’offset corrige un décalage constant, le scale corrige une proportion. Même si vous ne faites pas de calibration scientifique, ce modèle améliore la lisibilité et prépare votre code à des ajustements futurs.

En pratique, vous pouvez stocker ces paramètres dans des constantes, ou les charger depuis une configuration simple si votre environnement le permet. L’important est de garder la calibration visible et modifiable. Un calibrage caché est un bug latent.


## Étape 18 : stockage persistant (EEPROM/flash)

À un moment, vous voudrez garder un réglage entre deux redémarrages. C’est exactement le rôle de la mémoire persistante, comme l’EEPROM ou la flash. L’embarqué n’offre pas le confort d’un système de fichiers, donc la persistance doit être pensée avec parcimonie. La règle d’or est de limiter les écritures, car la mémoire flash s’use.

La stratégie minimale consiste à définir une petite structure de configuration, à l’écrire rarement, et à la relire au démarrage. Même si vous n’avez pas encore une API complète dans la stdlib, l’idée reste la même : un bloc de données stable, une fonction de lecture, une fonction d’écriture, et un checksum simple si possible. Le checksum n’est pas un luxe, c’est une barrière contre les corruptions silencieuses.

Un bon usage de la persistance est de stocker des paramètres d’étalonnage, un mode utilisateur, ou un dernier état connu. Mais il faut éviter d’écrire à chaque boucle. Un microcontrôleur qui écrit en permanence sur la flash finira par perdre la mémoire. La persistance est une ressource, pas un flux.

## Étape 19 : scheduling simple (timer + boucle)

Le scheduling en embarqué n’a pas besoin d’un RTOS pour être efficace. Un modèle simple “timer + boucle” suffit pour beaucoup d’applications. L’idée est de découper votre boucle principale en tâches, chacune avec sa propre cadence. Vous ne lancez pas une tâche à chaque itération, vous la lancez quand son timer l’autorise.

La structure classique est la suivante : vous lisez l’heure courante, vous comparez avec un timestamp, puis vous exécutez si le délai est passé. Cette approche est prévisible et facile à tester. Elle évite les `delay` bloquants, ce qui rend le programme plus réactif.

Même sans API complète, vous pouvez simuler cette logique avec des compteurs et des délais. L’important est d’être explicite sur les cadences : “lire le capteur toutes les 1000 ms, clignoter toutes les 200 ms”. Un scheduling clair est la base d’un système embarqué robuste.


## Étape 20 : exemple concret de persistance (calibration)

Voici une manière simple de penser la persistance d’un calibrage. L’idée est de stocker un offset et un scale, puis de les appliquer à chaque lecture. Le code exact dépend de l’API disponible, mais la structure conceptuelle est stable. Vous lisez la configuration au démarrage, vous appliquez, et vous réécrivez uniquement quand l’utilisateur change le calibrage.

Même si vous ne disposez pas d’un module EEPROM dans la stdlib, ce modèle guide votre implémentation. L’important est de rendre l’état persistant visible, et de limiter les écritures. Un calibrage écrit une fois par session est raisonnable ; un calibrage écrit à chaque boucle est dangereux.

## Étape 21 : scheduling simple en pseudo‑code Vitte

Ce pseudo‑code montre une boucle principale qui exécute deux tâches à des cadences différentes. Le principe est de stocker le dernier timestamp de chaque tâche, puis de comparer avec l’horloge courante.

```vit
let last_blink: u32 = 0
let last_read: u32 = 0

loop {
  let now = timer.millis()

  if now - last_blink >= 200 {
    // clignoter la LED
    last_blink = now
  }

  if now - last_read >= 1000 {
    // lire le capteur
    last_read = now
  }
}
```

Cette forme est simple, lisible, et extensible. Elle évite les `delay` bloquants et rend le programme plus réactif.


## Étape 22 : state machine pour les tâches

Quand un programme embarqué devient plus complexe, une simple boucle avec des timers peut être insuffisante. Une machine à états rend le flux explicite. Vous définissez des états stables et des transitions claires. Cela réduit les “if” imbriqués et rend le comportement prévisible.

Une machine à états simple pourrait gérer un cycle “init → read → process → sleep”. Chaque état fait une chose, puis passe explicitement au suivant. Le code devient plus long, mais beaucoup plus facile à comprendre et à déboguer.

## Étape 23 : tests embarqués sans matériel

Tester sans matériel est possible, mais il faut accepter un modèle simplifié. Vous pouvez simuler des capteurs en injectant des valeurs dans une fonction de lecture, ou créer un “driver” faux qui renvoie des valeurs prédictibles. L’objectif est de tester la logique, pas l’électronique.

Cette approche vous donne un filet de sécurité. Quand vous branchez enfin le matériel, vous savez déjà que la logique est correcte. Vous ne dépendez plus du hasard ou de l’intuition.


## Étape 24 : machine à états en pseudo‑code Vitte

Ce pseudo‑code illustre une machine à états simple. L’objectif est de rendre les transitions explicites. Chaque état fait une chose, puis choisit le prochain état.

```vit
pick State {
  Init
  Read
  Process
  Sleep
}

let state: State = State.Init

loop {
  match state {
    case State.Init {
      // initialiser les périphériques
      state = State.Read
    }
    case State.Read {
      // lire le capteur
      state = State.Process
    }
    case State.Process {
      // filtrer et préparer la sortie
      state = State.Sleep
    }
    otherwise {
      // temporiser
      state = State.Read
    }
  }
}
```

Cette structure est un excellent outil quand le flux devient trop complexe pour une simple boucle.

## Étape 25 : plan de tests simulés (étape par étape)

Un plan simple permet de valider la logique sans matériel. Vous injectez des valeurs connues, puis vous observez la sortie attendue.

Première étape : simuler une lecture stable et vérifier que le filtre ne modifie pas la valeur. Deuxième étape : injecter une valeur hors plage et vérifier qu’elle est rejetée. Troisième étape : simuler une séquence de lectures et vérifier que la moyenne glissante est correcte. Quatrième étape : simuler un capteur absent et vérifier que l’état “erreur” est atteint. Cinquième étape : simuler un retour à la normale et vérifier que le système récupère.

Ce plan n’est pas long, mais il couvre la majorité des bugs logiques. Il vous donne une base solide avant de passer au matériel.


## Étape 26 : profiling embarqué (simple et fiable)

Le profiling embarqué n’a pas besoin d’outils complexes. Une méthode simple consiste à mesurer des durées avec `timer.millis()` ou `timer.micros()` et à comparer les temps avant et après une section critique. L’objectif n’est pas la précision absolue, mais la compréhension relative : quelles parties de votre boucle coûtent le plus.

Un profiling minimal peut être réalisé en insérant des timestamps autour d’une fonction. Vous enregistrez la différence et vous l’émettez via un canal simple, comme la LED ou le port série. Même un chiffre approximatif vous aide à décider où optimiser.

## Étape 27 : gestion d’interruptions (concepts essentiels)

Les interruptions sont puissantes, mais dangereuses si elles sont mal utilisées. Une interruption doit être courte, rapide, et déterministe. Elle ne doit jamais contenir de logique complexe ou de blocage. Son rôle est de signaler qu’un événement est arrivé, pas de traiter tout l’événement.

Une stratégie saine est de définir un flag dans l’interruption, puis de traiter ce flag dans la boucle principale. Cela maintient la logique principale dans un flux lisible et évite les effets de bord imprévisibles. Ce modèle est une discipline, et c’est exactement ce qui rend un code embarqué stable.


## Étape 28 : debounce (anti‑rebond)

Les boutons mécaniques rebondissent. Cela signifie qu’un seul appui peut être lu comme plusieurs appuis. La correction n’est pas un détail, c’est une nécessité pour éviter un comportement erratique. La méthode la plus simple consiste à ignorer les changements pendant une fenêtre temporelle courte après la première détection. Le principe est de privilégier la fiabilité sur la réactivité extrême.

Même sans implémentation complète, l’idée est claire : vous lisez l’état, vous notez le temps, et vous n’acceptez pas un nouveau changement tant que le délai minimal n’est pas passé. Ce modèle protège la logique de vos états et évite des bugs très difficiles à diagnostiquer.

## Étape 29 : watchdog (redémarrage contrôlé)

Un watchdog est un mécanisme de sécurité. Si votre code se bloque, le watchdog redémarre le système. Cela peut sembler brutal, mais c’est souvent la meilleure stratégie sur un microcontrôleur. Le but n’est pas d’éviter tous les bugs, mais de garantir une récupération automatique quand un bug survient.

La discipline du watchdog est simple : vous “nourrissez” régulièrement le watchdog dans une boucle saine. Si la boucle se bloque, le watchdog déclenche un reset. Ce mécanisme doit être clair dans le code, afin que chaque mainteneur comprenne pourquoi un reset peut se produire.

## Étape 30 : bootloader et mise à jour de firmware

Le firmware n’est pas immuable. En pratique, vous devrez corriger des bugs ou ajouter des fonctionnalités. Cela implique une stratégie de mise à jour. Sur Arduino, vous utilisez souvent un bootloader existant, mais il est important de comprendre la logique : le bootloader est un petit programme qui charge le firmware réel.

Une bonne pratique consiste à garder un processus de mise à jour simple, documenté, et reproductible. La mise à jour est un moment de risque : si elle échoue, le système peut être inutilisable. Ce risque impose une discipline de build et de tests, même sur un projet “simple”.


## Étape 31 : sécurité électrique (pragmatique)

Le logiciel n’est pas isolé du matériel. Une erreur électrique peut détruire un capteur, ou pire, rendre votre diagnostic impossible. Cela signifie que la discipline logicielle doit être accompagnée d’une discipline matérielle. Utiliser des résistances adaptées, respecter les niveaux de tension, et éviter les courts‑circuits est une condition de réussite. Même si vous êtes côté logiciel, vous devez comprendre ces contraintes.

Sur le plan logiciel, cela se traduit par une prudence de configuration. Par exemple, éviter de configurer une pin en sortie si elle est câblée sur un bus partagé, ou éviter d’activer des modes qui mettent le matériel en conflit. La sécurité électrique est aussi une question de séquence : initialiser d’abord, puis activer, jamais l’inverse.

## Étape 32 : diagnostic en production

Quand un firmware est déployé, vous n’avez plus de terminal ni d’IDE. Le diagnostic doit donc être pensé en amont. La stratégie la plus simple est de réserver un canal de signalisation. Une LED peut être suffisante. Un port série minimal peut suffire. L’essentiel est d’avoir un signal stable qui indique l’état général et les erreurs critiques.

Un diagnostic en production n’a pas besoin d’être verbeux, il doit être fiable. Un code d’erreur succinct est préférable à un message long et fragile. L’objectif est de permettre une récupération rapide, même sans outils sophistiqués.

