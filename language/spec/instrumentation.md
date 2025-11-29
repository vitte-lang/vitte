# Instrumentation et meta-programmation

Mots-clés : `probe`, `trace`, `watch`, `hook`, `meta`, `pragma`, `emit`, `explain`.

## Objectifs
* Faciliter l’observabilité (traces, sondes).
* Offrir des directives de compilation simples (`pragma`).
* Permettre l’émission contrôlée d’artefacts (`emit`, `explain`).

## Exemples

```vitte
probe io_latency(sample):
    trace "io-latency" (sample.id, sample.time)

pragma optimize(level=2)

meta register("driver.usb", version="1.0")
```

Chaque directive peut se suffire d’une seule ligne ou ouvrir un bloc complet
pour instrumentation avancée.
