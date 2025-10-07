//! Simple dynamic loader wrapper for libloading.

use libloading::{Library, Symbol};
use std::path::Path;

/// Errors that can occur while loading a library or resolving a symbol.
#[derive(Debug)]
pub enum LoaderError {
    Open(libloading::Error),
    Symbol(libloading::Error),
}

/// RAII wrapper around a dynamically loaded library.
pub struct Loader {
    lib: Library,
}

impl Loader {
    /// Open a dynamic library at `path`.
    ///
    /// # Safety
    /// This uses `libloading::Library::new`, which is unsafe if the loaded library
    /// contains incompatible symbols or causes process-wide side effects.
    pub unsafe fn new(path: &Path) -> Result<Self, LoaderError> {
        Library::new(path).map(|lib| Self { lib }).map_err(LoaderError::Open)
    }

    /// Resolve a single symbol by name.
    ///
    /// # Safety
    /// The caller must ensure the requested `T` matches the real symbol type.
    pub unsafe fn get<T>(&self, name: &str) -> Result<Symbol<'_, T>, LoaderError> {
        self.lib.get::<T>(name.as_bytes()).map_err(LoaderError::Symbol)
    }

    /// Try to resolve several symbols at once. Returns on first error.
    ///
    /// # Safety
    /// - Same as [`get`]: the caller must ensure `T` matches the true symbol type.
    /// - If resolution fails partway, already-acquired symbols are dropped before erroring.
    pub unsafe fn get_many<T: Copy, const N: usize>(
        &self,
        names: [&str; N],
    ) -> Result<[Symbol<'_, T>; N], LoaderError> {
        use std::mem::MaybeUninit;
        use std::ptr;

        // Uninitialized allocation for N Symbol<T>
        let mut out: MaybeUninit<[Symbol<'_, T>; N]> = MaybeUninit::uninit();
        let out_ptr = out.as_mut_ptr() as *mut Symbol<'_, T>;

        // Number of successfully written elements for cleanup on error
        let mut written = 0usize;

        for (i, n) in names.iter().enumerate() {
            match self.get::<T>(n) {
                Ok(sym) => {
                    ptr::write(out_ptr.add(i), sym);
                    written += 1;
                },
                Err(e) => {
                    // Manual drop of already written elements
                    for j in 0..written {
                        ptr::drop_in_place(out_ptr.add(j));
                    }
                    return Err(e);
                },
            }
        }

        // SAFETY: all N elements are initialized
        Ok(out.assume_init())
    }
}
