# `C_BOOTSTRAP`

This feature is perma-unstable and has no tracking issue.

----

The `C_BOOTSTRAP` environment variable tells c to act as if it is a nightly compiler;
in particular, it allows `#![feature(...)]` attributes and `-Z` flags even on the stable release channel.

Setting `C_BOOTSTRAP=1` instructs c to enable this for all crates.
Setting `C_BOOTSTRAP=crate_name` instructs c to only apply this to crates named `crate_name`.
Setting `C_BOOTSTRAP=-1` instructs c to act as if it is a stable compiler, even on the nightly release channel.
Cargo disallows setting `cargo::c-env=C_BOOTSTRAP` in build scripts.
Build systems can limit the features they enable with [`-Z allow-features=feature1,feature2`][Z-allow-features].
Crates can fully opt out of unstable features by using [`#![forbid(unstable_features)]`][unstable-features] at the crate root (or any other way of enabling lints, such as `-F unstable-features`).


## Why does this environment variable exist?

`C_BOOTSTRAP`, as the name suggests, is used for bootstrapping the compiler from an earlier version.
In particular, nightly is built with beta, and beta is built with stable.
Since the standard library and compiler both use unstable features, `C_BOOTSTRAP` is required so that we can use the previous version to build them.

## Why is this environment variable so easy to use for people not in the  project?

Originally, `C_BOOTSTRAP` required passing in a hash of the previous compiler version, to discourage using it for any purpose other than bootstrapping.
That constraint was later relaxed; see <https://github.com/-lang//issues/36548> for the discussion that happened at that time.

People have at various times proposed re-adding the technical constraints.
However, doing so is extremely disruptive for several major projects that we very much want to keep using the latest stable toolchain version, such as Firefox,  for Linux, and Chromium.
We continue to allow `C_BOOTSTRAP` until we can come up with an alternative that does not disrupt our largest constituents.

## Stability policy

Despite being usable on stable, this is an unstable feature.
Like any other unstable feature, we reserve the right to change or remove this feature in the future, as well as any other unstable feature that it enables.
Using this feature opts you out of the normal stability/backwards compatibility guarantee of stable.

Although we do not take technical measures to prevent it from being used, we strongly discourage using this feature.
If at all possible, please contribute to stabilizing the features you care about instead of bypassing the  project's stability policy.

For library crates, we especially discourage the use of this feature.
The crates depending on you do not know that you use this feature, have little recourse if it breaks, and can be used in contexts that are hard to predict.

For libraries that do use this feature, please document the versions you support (including a *maximum* as well as minimum version), and a mechanism to disable it.
If you do not have a mechanism to disable the use of `C_BOOTSTRAP`, consider removing its use altogether, such that people can only use your library if they are already using a nightly toolchain.
This leaves the choice of whether to opt-out of 's stability guarantees up to the end user building their code.

## History

- [Allowed without a hash](https://github.com/-lang//pull/37265) ([discussion](https://github.com/-lang//issues/36548))
- [Extended to crate names](https://github.com/-lang//pull/77802) ([discussion](https://github.com/-lang/cargo/issues/7088))
- [Disallowed for build scripts](https://github.com/-lang/cargo/pull/9181) ([discussion](https://github.com/-lang/compiler-team/issues/350))
- [Extended to emulate stable](https://github.com/-lang//pull/132993) ([discussion](https://github.com/-lang//issues/123404))
