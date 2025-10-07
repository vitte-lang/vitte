//! vitte-plugin — Plugin runtime descriptor for Vitte/Vitl dynamic modules.
//!
//! Provides a safe Rust interface for declaring and registering plugin metadata.

use std::ffi::CString;
use std::os::raw::c_char;
use std::sync::OnceLock;
use std::time::Instant;

/// Represents metadata describing a plugin for the Vitte runtime.
#[repr(C)]
pub struct VittePluginDesc {
    /// Plugin name (UTF-8 C string)
    pub name: *const c_char,
    /// Plugin version (UTF-8 C string)
    pub version: *const c_char,
    /// Author (UTF-8 C string)
    pub author: *const c_char,
    /// License (UTF-8 C string)
    pub license: *const c_char,
    /// Description (UTF-8 C string)
    pub description: *const c_char,
}

// SAFETY: All pointers reference static C strings leaked for the duration of the program.
unsafe impl Send for VittePluginDesc {}
unsafe impl Sync for VittePluginDesc {}

static DESC: OnceLock<VittePluginDesc> = OnceLock::new();

/// A helper type representing the plugin’s global state.
pub struct PluginState {
    pub started_at: Instant,
}

impl PluginState {
    /// Creates a new plugin state.
    pub fn new() -> Self {
        Self {
            started_at: Instant::now(),
        }
    }
}

/// Initializes the plugin descriptor with provided metadata.
pub fn init_plugin(
    name: &str,
    version: &str,
    author: &str,
    license: &str,
    description: &str,
) -> &'static VittePluginDesc {
    let desc = VittePluginDesc {
        name: CString::new(name).unwrap().into_raw(),
        version: CString::new(version).unwrap().into_raw(),
        author: CString::new(author).unwrap().into_raw(),
        license: CString::new(license).unwrap().into_raw(),
        description: CString::new(description).unwrap().into_raw(),
    };
    DESC.get_or_init(|| desc)
}

/// Returns the global plugin descriptor if initialized.
pub fn get_plugin_desc() -> Option<&'static VittePluginDesc> {
    DESC.get()
}

/// Example function simulating plugin logic.
pub fn run_plugin_logic(state: &PluginState) {
    let _uptime = state.started_at.elapsed();
    println!("Plugin running for {:.2?}", _uptime);
}

/// Example entry point — registers and runs the plugin.
#[no_mangle]
pub extern "C" fn vitte_plugin_entry() {
    let state = PluginState::new();

    let desc = init_plugin(
        "ExamplePlugin",
        "0.1.0",
        "Vitte Authors",
        "MIT",
        "Example plugin demonstrating descriptor registration.",
    );

    println!(
        "Registered plugin: {} v{} by {}",
        unsafe { std::ffi::CStr::from_ptr(desc.name).to_string_lossy() },
        unsafe { std::ffi::CStr::from_ptr(desc.version).to_string_lossy() },
        unsafe { std::ffi::CStr::from_ptr(desc.author).to_string_lossy() }
    );

    run_plugin_logic(&state);
}
