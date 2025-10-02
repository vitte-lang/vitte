// src/lib.rs
//! vitte-fronted-wasm — pont WebAssembly pour Vitte.
//! Exporte des helpers JS/DOM, init de logs, et gestion des erreurs.

use wasm_bindgen::prelude::*;

#[cfg(feature = "panic-hook")]
#[inline]
fn set_panic_hook() {
    console_error_panic_hook::set_once();
}

#[cfg(not(feature = "panic-hook"))]
#[inline]
fn set_panic_hook() {}

#[wasm_bindgen(start)]
pub fn start() {
    set_panic_hook();
    // Log vers console.* via log crate
    let _ = wasm_logger::init(wasm_logger::Config::default());
    log::info!("vitte-fronted-wasm: start()");
}

/// Renvoie un message de bienvenue sérialisé JSON.
#[wasm_bindgen]
pub fn greet(name: &str) -> String {
    #[derive(serde::Serialize)]
    struct Msg<'a> { ok: bool, hello: &'a str }
    serde_json::to_string(&Msg { ok: true, hello: name }).unwrap_or_else(|_| "{\"ok\":false}".into())
}

/// Écrit `text` dans l'élément DOM avec l’`id` donné.
#[wasm_bindgen]
pub fn set_text_by_id(id: &str, text: &str) -> Result<(), JsValue> {
    #[cfg(feature = "web")]
    {
        let window = web_sys::window().ok_or_else(|| js_sys::Error::new("no window"))?;
        let doc = window.document().ok_or_else(|| js_sys::Error::new("no document"))?;
        let el = doc
            .get_element_by_id(id)
            .ok_or_else(|| js_sys::Error::new("element not found"))?;
        el.set_text_content(Some(text));
        Ok(())
    }
    #[cfg(not(feature = "web"))]
    {
        let _ = (id, text);
        Err(js_sys::Error::new("feature `web` disabled").into())
    }
}

/// Mesure le temps d’une closure JS (Function) en ms et le retourne.
#[wasm_bindgen]
pub fn time_fn(f: &js_sys::Function) -> Result<f64, JsValue> {
    #[cfg(feature = "web")]
    {
        let perf = web_sys::window()
            .and_then(|w| w.performance())
            .ok_or_else(|| js_sys::Error::new("no performance"))?;
        let t0 = perf.now();
        let this = JsValue::NULL;
        let _ = f.call0(&this)?;
        let dt = perf.now() - t0;
        Ok(dt)
    }
    #[cfg(not(feature = "web"))]
    {
        let _ = f;
        Err(js_sys::Error::new("feature `web` disabled").into())
    }
}

/// Log niveau info vers la console du navigateur.
#[wasm_bindgen]
pub fn log_info(msg: &str) {
    log::info!("{msg}");
}