# RFC 0013 — Security Threat Model

## Résumé
Ce RFC définit le **modèle de menaces de sécurité** pour le langage **Vitte**, son compilateur, sa machine virtuelle et son écosystème.  
L’objectif est d’identifier les risques, d’établir des protections par défaut et de guider l’évolution sécurisée du langage.

## Motivation
Un langage moderne ne peut ignorer la sécurité :  
- prévenir les vulnérabilités mémoire (buffer overflow, use-after-free),  
- éviter les injections et escalades dans la VM,  
- assurer la confiance des développeurs et entreprises,  
- garantir la résilience des applications critiques (systèmes embarqués, finance, cloud).  

## Design proposé

### Principes fondamentaux
1. **Sûreté mémoire** par ownership et borrow checker.  
2. **Isolation des modules** (sandboxing, contrôle des imports).  
3. **Défense en profondeur** : multiples couches de sécurité (compilateur, VM, runtime).  
4. **Sécurité par défaut** : les comportements non sûrs doivent être explicites (`unsafe`).  

### Surfaces d’attaque identifiées
- **Au niveau langage** :  
  - Pointeurs bruts mal utilisés.  
  - Erreurs de concurrence (`data races`).  
  - API FFI non sûres.  

- **Au niveau VM** :  
  - Exécution de bytecode non vérifié.  
  - Évasion de sandbox.  
  - Attaques par consommation excessive de ressources (DoS).  

- **Au niveau écosystème** :  
  - Dépendances malveillantes via `vitpm`.  
  - Chaîne d’approvisionnement compromise (supply chain).  
  - Vulnérabilités dans les bibliothèques standard.  

### Contremesures

#### Dans le compilateur
- Vérification stricte des types et emprunts.  
- Détection des comportements indéfinis.  
- Lints de sécurité (`#[deny(unsafe_op_in_unsafe_fn)]`).  

#### Dans la VM
- Vérification du bytecode avant exécution.  
- Limitation des ressources (mémoire, temps CPU).  
- Sandboxing pour isoler les modules non fiables.  

#### Dans l’écosystème
- Signatures cryptographiques des paquets `vitpm`.  
- Vérification d’intégrité avant installation.  
- Audit automatisé des dépendances (`vitpm audit`).  

### Exemple : FFI sécurisé
```vitte
unsafe extern(c) fn malloc(size: usize) -> *mut u8

fn main() {
    let p = unsafe { malloc(1024) }
    // pointeur marqué comme potentiellement dangereux
}
```

### Exemple : sandbox VM
```sh
vitvm --sandbox --max-mem=64MB --max-cpu=1s script.vbc
```

### Politique de mise à jour
- CVE attribués aux vulnérabilités majeures.  
- Correctifs de sécurité prioritaires (release rapides).  
- Versions mineures garantissent rétrocompatibilité sécuritaire.  

## Alternatives considérées
- **Pas de modèle de menaces explicite** : rejeté pour manque de maturité.  
- **Sécurité confiée uniquement aux développeurs** : rejetée car source de vulnérabilités massives.  
- **VM sans sandbox** : rejetée pour exécutions non fiables (cloud, WASM).  

## Impact et compatibilité
- Impact majeur sur la fiabilité de Vitte.  
- Compatible avec ownership, async, et FFI.  
- Alourdit légèrement le runtime (sandbox, vérifications).  
- Augmente la confiance et l’adoption en milieu professionnel.  

## Références
- [Rust Unsafe Code Guidelines](https://rust-lang.github.io/unsafe-code-guidelines/)  
- [OWASP Threat Modeling](https://owasp.org/www-community/Threat_Modeling)  
- [Supply Chain Security](https://slsa.dev/)  
- [WASM Security](https://webassembly.org/security/)  
