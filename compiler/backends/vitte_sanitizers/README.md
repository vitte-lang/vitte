# Vitte Sanitizers Backend

Ce dossier contient l’implémentation complète du backend **vitte_sanitizers** du compilateur **Vitte**.

Ce backend fournit une **instrumentation dynamique au niveau MIR** afin de détecter, à l’exécution, différents types d’erreurs :
- erreurs mémoire
- comportements indéfinis
- data races
- violations de synchronisation

L’architecture est **backend-agnostique** (LLVM, Cranelift, etc.) et repose sur des **intrinsics MIR** reliées à des **runtimes dédiés**.

---

## Objectifs

- Instrumenter le MIR sans dépendre du codegen cible
- Séparer clairement :
  - configuration
  - instrumentation
  - runtime
- Fournir une API stable au driver du compilateur
- Être extensible sans casser l’ABI

---

## Sanitizers supportés

| Sanitizer | Nom | Description |
|----------|-----|-------------|
| ASan | AddressSanitizer | Détecte les accès mémoire invalides (heap/stack) |
| TSan | ThreadSanitizer | Détecte les data races et erreurs de synchronisation |
| UBSan | UndefinedBehaviorSanitizer | Détecte les comportements indéfinis (overflow, div0, casts, etc.) |

---


---

## Flux d’exécution

1. Le **driver** crée des `SanitizerOptions`
2. `vitte_sanitizers::run_sanitizers` est appelé
3. Les passes MIR sont exécutées dans l’ordre :
   - AddressSanitizer
   - ThreadSanitizer
   - UndefinedBehaviorSanitizer
4. Des **intrinsics MIR** sont injectées :
   - `__asan_*`
   - `__tsan_*`
   - `__ubsan_*`
5. Le codegen résout ces symboles vers le **runtime sanitizers**

---

## Configuration

La configuration se fait via `SanitizerOptions` :

```vit
let opts = SanitizerOptions::address()
