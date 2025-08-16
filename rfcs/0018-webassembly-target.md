# RFC 0018 — WebAssembly Target

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0018  

---

## Résumé
Ajoute WebAssembly (WASM) comme cible de compilation.  

## Motivation
Permettre à Vitte de tourner dans les navigateurs et environnements sandbox.  

## Détails
- Backend LLVM → wasm32.  
- Interop JS via `extern(js)`.  

### Exemple
```vitte
extern(js) fn alert(s: string);

fn main() {
    alert("Hello from Vitte!");
}
```

## Alternatives
WASM via transpile JS (inefficace).  

## Impact
Ouvre le monde web à Vitte.  

## Adoption
Phase beta.  

## Références
- WebAssembly spec  
- Emscripten  
- Rust WASM target  
