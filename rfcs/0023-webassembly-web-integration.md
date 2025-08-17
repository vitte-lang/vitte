# RFC 0023 — WebAssembly and Web Integration

## Résumé
Ce RFC définit le support de **WebAssembly (Wasm)** et de l’intégration web dans le langage **Vitte**.  
L’objectif est de permettre la compilation de Vitte vers le navigateur, Node.js et les runtimes Wasm, tout en offrant des bindings ergonomiques vers le JavaScript et le DOM.

## Motivation
Le Web est une plateforme incontournable :  
- WebAssembly offre des performances quasi natives dans le navigateur,  
- permet d’écrire du code réutilisable côté client et serveur,  
- ouvre la voie à des applications embarquées dans le cloud et l’IoT.  

## Design proposé

### Cible `wasm32`
- Ajout via :  
```sh
vitpm target add wasm32-unknown-unknown
```
- Génère un module `.wasm` + bindings JS.  
- Compatible avec navigateur et Node.js.  

### Interopérabilité JS
- Module `std::wasm::js`.  
- Appel de fonctions JS depuis Vitte :  
```vitte
@wasm_import("console.log")
fn log(msg: &str)

fn main() {
    log("Bonjour depuis Vitte en WebAssembly !")
}
```

- Export de fonctions Vitte utilisables en JS :  
```vitte
@wasm_export
fn add(a: i32, b: i32) -> i32 {
    a + b
}
```

### Accès DOM
- Binding `vitdom` pour interagir avec le navigateur :  
```vitte
use vitdom

fn main() {
    let doc = vitdom::document()
    let el = doc.create_element("h1")
    el.set_text("Hello Vitte + Wasm!")
    doc.body().append(el)
}
```

### Async et Promises
Interop avec les Promises JS via `async`.  
```vitte
async fn fetch_data(url: &str) -> String {
    let resp = js::fetch(url).await
    resp.text().await
}
```

### Packaging
- `vitpm build --target wasm32` génère :  
  - `module.wasm`  
  - `bindings.js`  
  - `package.json` (optionnel)  

### Exemple de projet minimal
```sh
vitte new wasm-hello --target wasm32
cd wasm-hello
vitpm build --release
```

→ Produit un module WebAssembly directement exécutable dans une page HTML.  

### Intégration CI/CD
- Docker image `vitte/wasm`.  
- Tests automatiques dans navigateur headless (`vitpm test --wasm`).  

## Alternatives considérées
- **Ne pas supporter WebAssembly** : rejeté, car limite l’usage de Vitte.  
- **Interop uniquement via C FFI** : rejeté pour ergonomie insuffisante.  
- **Libs externes uniquement** : rejeté pour manque d’uniformité.  

## Impact et compatibilité
- Impact fort : permet d’écrire des apps web modernes en Vitte.  
- Compatible avec Node.js, Deno, et navigateurs modernes.  
- Ouvre la voie à des frameworks fullstack en Vitte.  

## Références
- [WebAssembly.org](https://webassembly.org/)  
- [Rust Wasm Bindgen](https://github.com/rustwasm/wasm-bindgen)  
- [Emscripten](https://emscripten.org/)  
- [WASI (WebAssembly System Interface)](https://wasi.dev/)  
