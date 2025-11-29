# Extensions optionnelles

Mots-clés : `table`, `plot`, `ui`.

Ces blocs servent à décrire des rendus légers (tableaux de bord, graphiques,
interfaces scolaires) directement dans un module Vitte.

```vitte
table metrics(data):
    column "Device"
    column "Latency"
    rows data

plot trend(points):
    axis x "time"
    axis y "temp"
    data points
```

Un bloc `ui` peut intégrer des éléments interactifs basiques (`button`,
`slider`, etc.) pilotés par les fonctions Vitte du module courant.
