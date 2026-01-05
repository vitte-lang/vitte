This crate is currently developed in-tree together with the compiler.

Our goal is to start publishing `stable_mir` into crates.io.
Until then, users will use this as any other c crate, by installing
the up component `c-dev`, and declaring `stable-mir` as an external crate.

See the StableMIR ["Getting Started"](https://-lang.github.io/project-stable-mir/getting-started.html)
guide for more information.

## Stable MIR Design

The stable-mir will follow a similar approach to proc-macro2. Its
implementation is split between two main crates:

- `stable_mir`: Public crate, to be published on crates.io, which will contain
the stable data structure as well as calls to `c_smir` APIs. The
translation between stable and internal constructs will also be done in this crate,
however, this is currently implemented in the `c_smir` crate.[^translation].
- `c_smir`: This crate implements the public APIs to the compiler.
It is responsible for gathering all the information requested, and providing
the data in its unstable form.

[^translation]: This is currently implemented in the `c_smir` crate,
but we are working to change that.

I.e.,
tools will depend on `stable_mir` crate,
which will invoke the compiler using APIs defined in `c_smir`.

I.e.:

```
    ┌──────────────────────────────────┐           ┌──────────────────────────────────┐
    │   External Tool     ┌──────────┐ │           │ ┌──────────┐    Compiler     │
    │                     │          │ │           │ │          │                     │
    │                     │stable_mir| │           │ │c_smir│                     │
    │                     │          │ ├──────────►| │          │                     │
    │                     │          │ │◄──────────┤ │          │                     │
    │                     │          │ │           │ │          │                     │
    │                     │          │ │           │ │          │                     │
    │                     └──────────┘ │           │ └──────────┘                     │
    └──────────────────────────────────┘           └──────────────────────────────────┘
```

More details can be found here:
https://hackmd.io/XhnYHKKuR6-LChhobvlT-g?view
