//! vitte-wasm — bindings WebAssembly/JS pour Vitte.
//!
//! Exports (JS/TS) :
//! - init() / version()
//! - is_vitbc(bytes) -> bool                      (check rapide "VITBC\0")
//! - header_info(bytes) -> { version: u16 }       (lecture rapide du header)
//! - inspect(bytes) -> InspectReport              (parse + CRC validé)
//! - inspect_with_names(bytes, max) -> InspectReport { names[..max] }
//! - inspect_url(url) -> Promise<InspectReport>   (fetch + parse)
//! - crc32(bytes) -> u32                          (crc extrait après parse)
//! - code(bytes) -> Uint8Array                    (section CODE, décompressée si feature `zstd` activée côté vitte-vitbc)
//! - names(bytes) -> string[]                     (section NAME)
//! - stringify(bytes) -> string                   (résumé humain pour debug)
//! - run_bytes/run_url → stubs tant que feature `vm` non activée
//!
//! Build :
//!   rustup target add wasm32-unknown-unknown
//!   cargo build -p vitte-wasm --target wasm32-unknown-unknown --release
//!   wasm-bindgen --target web --no-typescript --out-dir dist target/wasm32-unknown-unknown/release/vitte_wasm.wasm

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use core::fmt;
use js_sys::Uint8Array;
use wasm_bindgen::prelude::*;
use wasm_bindgen_futures::JsFuture;

use vitte_core::CoreError;
use vitte_vitbc::Module;

/// Rapport compact côté JS/TS.
#[derive(serde::Serialize, serde::Deserialize, Debug, Clone)]
pub struct InspectReport {
    /// Version VITBC.
    pub version: u16,
    /// Nombre d'entrées par section.
    pub ints: usize,
    /// Nombre de flottants.
    pub floats: usize,
    /// Nombre de chaînes.
    pub strings: usize,
    /// Taille DATA (octets).
    pub data_len: usize,
    /// Taille CODE (octets, après décompression éventuelle).
    pub code_len: usize,
    /// Nombre de noms (debug).
    pub names: usize,
    /// Noms tronqués (optionnels, remplis par inspect_with_names).
    pub sample_names: Option<Vec<String>>,
}

impl From<&Module> for InspectReport {
    fn from(m: &Module) -> Self {
        Self {
            version: m.version,
            ints: m.ints.len(),
            floats: m.floats.len(),
            strings: m.strings.len(),
            data_len: m.data.len(),
            code_len: m.code.len(),
            names: m.names.len(),
            sample_names: None,
        }
    }
}

/* ─────────────────────────── JS utils ─────────────────────────── */

fn js_err(msg: impl fmt::Display) -> JsValue {
    JsValue::from_str(&msg.to_string())
}

fn core_err(e: CoreError) -> JsValue {
    JsValue::from_str(&e.to_string())
}

/* ─────────────────────────── Exports ─────────────────────────── */

/// Initialise le runtime WASM (panic hook + logs console si feature `console`).
#[wasm_bindgen]
pub fn init() {
    #[cfg(feature = "console")]
    {
        console_error_panic_hook::set_once();
        let _ = console_log::init_with_level(log::Level::Info);
        log::info!("vitte-wasm: init()");
    }
}

/// (optionnel) auto-init quand le module est chargé côté web.
#[wasm_bindgen(start)]
pub fn _auto_start() {
    // Pas de panique si `console` n’est pas activée.
    #[cfg(feature = "console")]
    {
        let _ = std::panic::catch_unwind(|| init());
    }
}

/// Version du crate.
#[wasm_bindgen]
pub fn version() -> String {
    env!("CARGO_PKG_VERSION").to_string()
}

/// Vérifie rapidement la signature "VITBC\0".
#[wasm_bindgen]
pub fn is_vitbc(bytes: Uint8Array) -> bool {
    let v = bytes.to_vec();
    v.len() >= 6 && &v[..6] == b"VITBC\0"
}

/// Lit juste la version depuis le header (sans parser toutes les sections).
#[wasm_bindgen]
pub fn header_info(bytes: Uint8Array) -> Result<JsValue, JsValue> {
    let v = bytes.to_vec();
    if v.len() < 8 || &v[..6] != b"VITBC\0" {
        return Err(js_err("not a VITBC file"));
    }
    // u16 LE juste après "VITBC\0"
    let version = u16::from_le_bytes([v[6], v[7]]);
    let obj = js_sys::Object::new();
    js_sys::Reflect::set(&obj, &"version".into(), &JsValue::from_f64(version as f64))?;
    Ok(obj.into())
}

/// Inspecte un buffer `.vitbc` (Uint8Array) et renvoie un `InspectReport` (CRC validé).
#[wasm_bindgen]
pub fn inspect(bytes: Uint8Array) -> Result<JsValue, JsValue> {
    let vec = bytes.to_vec();
    let module = Module::from_bytes(&vec).map_err(core_err)?;
    let rep = InspectReport::from(&module);
    serde_wasm_bindgen::to_value(&rep).map_err(js_err)
}

/// Variante : inclut jusqu’à `max_names` noms (debug) dans le rapport.
#[wasm_bindgen]
pub fn inspect_with_names(bytes: Uint8Array, max_names: u32) -> Result<JsValue, JsValue> {
    let vec = bytes.to_vec();
    let module = Module::from_bytes(&vec).map_err(core_err)?;
    let mut rep = InspectReport::from(&module);
    let take = (max_names as usize).min(module.names.len());
    rep.sample_names = Some(module.names.iter().take(take).cloned().collect());
    serde_wasm_bindgen::to_value(&rep).map_err(js_err)
}

/// `fetch(url)` puis `inspect` — renvoie une `Promise<InspectReport>`.
#[wasm_bindgen]
pub async fn inspect_url(url: String) -> Result<JsValue, JsValue> {
    let bytes = fetch_bytes(&url).await?;
    let module = Module::from_bytes(&bytes).map_err(core_err)?;
    let rep = InspectReport::from(&module);
    serde_wasm_bindgen::to_value(&rep).map_err(js_err)
}

/// Calcule/retourne le CRC reporté après parse (erreur si invalide).
#[wasm_bindgen]
pub fn crc32(bytes: Uint8Array) -> Result<u32, JsValue> {
    let vec = bytes.to_vec();
    let module = Module::from_bytes(&vec).map_err(core_err)?;
    Ok(module.crc32)
}

/// Extrait la section CODE (décompressée si `zstd` activée côté vitte-vitbc).
#[wasm_bindgen]
pub fn code(bytes: Uint8Array) -> Result<Uint8Array, JsValue> {
    let vec = bytes.to_vec();
    let module = Module::from_bytes(&vec).map_err(core_err)?;
    let out = Uint8Array::new_with_length(module.code.len() as u32);
    out.copy_from(&module.code);
    Ok(out)
}

/// Renvoie la liste des noms (debug) sous forme de tableau JS.
#[wasm_bindgen]
pub fn names(bytes: Uint8Array) -> Result<JsValue, JsValue> {
    let vec = bytes.to_vec();
    let module = Module::from_bytes(&vec).map_err(core_err)?;
    serde_wasm_bindgen::to_value(&module.names).map_err(js_err)
}

/// Résumé humain (multi-lignes) pour debug rapide.
#[wasm_bindgen]
pub fn stringify(bytes: Uint8Array) -> Result<String, JsValue> {
    let v = bytes.to_vec();
    let m = Module::from_bytes(&v).map_err(core_err)?;
    let s = format!(
        "VITBC v{}\nINTS: {}\nFLTS: {}\nSTRS: {}\nDATA: {} bytes\nCODE: {} bytes\nNAMES: {}\nCRC32: 0x{:08X}",
        m.version, m.ints.len(), m.floats.len(), m.strings.len(), m.data.len(), m.code.len(), m.names.len(), m.crc32
    );
    Ok(s)
}

/// Exécute un module depuis un buffer — **stub** tant que la feature `vm` n'est pas active.
#[wasm_bindgen]
pub fn run_bytes(_bytes: Uint8Array) -> Result<JsValue, JsValue> {
    #[cfg(feature = "vm")]
    {
        // TODO: brancher vitte-vm ici quand l'API est stabilisée.
        // let code = _bytes.to_vec();
        // let rep = vitte_vm::run(&code).map_err(|e| js_err(e))?;
        // return serde_wasm_bindgen::to_value(&rep).map_err(js_err);
    }
    Err(js_err("vitte-wasm: feature `vm` non activée"))
}

/// `fetch(url)` puis `run_bytes` — **stub** idem.
#[wasm_bindgen]
pub async fn run_url(_url: String) -> Result<JsValue, JsValue> {
    #[cfg(feature = "vm")]
    {
        // TODO: idem ci-dessus
    }
    Err(js_err("vitte-wasm: feature `vm` non activée"))
}

/* ─────────────────────────── Fetch helper ─────────────────────────── */

async fn fetch_bytes(url: &str) -> Result<Vec<u8>, JsValue> {
    let window = web_sys::window().ok_or_else(|| js_err("no global `window`"))?;
    let resp_promise = window.fetch_with_str(url);
    let resp_value = JsFuture::from(resp_promise).await?;
    let resp: web_sys::Response = resp_value.dyn_into()?;

    if !resp.ok() {
        return Err(js_err(format!(
            "HTTP {} {}",
            resp.status(),
            resp.status_text()
        )));
    }

    let ab_promise = resp
        .array_buffer()
        .map_err(|e| js_err(format!("arrayBuffer() failed: {:?}", e)))?;
    let ab_value = JsFuture::from(ab_promise).await?;
    let ab: js_sys::ArrayBuffer = ab_value.dyn_into()?;

    let u8 = Uint8Array::new(&ab);
    Ok(u8.to_vec())
}

/* ─────────────────────────── Tests wasm (basics) ─────────────────────────── */

#[cfg(all(test, target_arch = "wasm32"))]
mod wasm_tests {
    use super::*;
    use wasm_bindgen_test::wasm_bindgen_test;

    wasm_bindgen_test::wasm_bindgen_test_configure!(run_in_browser);

    #[wasm_bindgen_test]
    fn version_works() {
        let v = version();
        assert!(!v.is_empty());
    }
}
