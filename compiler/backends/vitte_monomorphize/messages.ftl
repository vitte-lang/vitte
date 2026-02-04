# ============================================================
# Vitte — Monomorphization Diagnostics
# File: messages.ftl
#
# Conventions:
# - mono-* : monomorphization errors
# - Keys are stable (do not rename lightly)
# - Messages are user-facing (compiler diagnostics)
# ============================================================


# ------------------------------------------------------------
# Collection phase
# ------------------------------------------------------------

mono-collect-missing-function =
    internal error: referenced function with id { $fn_id } was not found during monomorphization collection.

mono-collect-missing-type =
    internal error: referenced type with id { $type_id } was not found during monomorphization collection.


# ------------------------------------------------------------
# Instantiation phase
# ------------------------------------------------------------

mono-instantiate-missing-function =
    internal error: cannot instantiate function { $fn_key } because the generic definition was not found.

mono-instantiate-missing-type =
    internal error: cannot instantiate type { $type_key } because the generic definition was not found.

mono-instantiate-duplicate-function =
    internal error: function { $fn_key } was instantiated more than once.

mono-instantiate-duplicate-type =
    internal error: type { $type_key } was instantiated more than once.


# ------------------------------------------------------------
# Rewrite phase
# ------------------------------------------------------------

mono-rewrite-missing-function-instance =
    internal error: no concrete instance found for function specialization { $fn_key }.

mono-rewrite-missing-type-instance =
    internal error: no concrete instance found for type specialization { $type_key }.

mono-rewrite-generic-leftover =
    internal error: generic construct { $item } survived monomorphization rewrite phase.


# ------------------------------------------------------------
# Type substitution
# ------------------------------------------------------------

mono-type-invalid-generic-index =
    internal error: invalid generic parameter index { $index } during type substitution.

mono-type-unmangleable =
    internal error: cannot mangle type { $type } after monomorphization.


# ------------------------------------------------------------
# Context validation
# ------------------------------------------------------------

mono-context-uninstantiated-function =
    internal error: function specialization { $fn_key } was collected but never instantiated.

mono-context-uninstantiated-type =
    internal error: type specialization { $type_key } was collected but never instantiated.


# ------------------------------------------------------------
# Name mangling
# ------------------------------------------------------------

mono-mangle-collision =
    internal error: name mangling collision detected for symbol { $name }.


# ------------------------------------------------------------
# Fallback / unknown
# ------------------------------------------------------------

mono-internal-error =
    internal error during monomorphization: { $details }.
