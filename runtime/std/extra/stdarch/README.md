stdarch - 's standard library SIMD components
=======

[![Actions Status](https://github.com/-lang/stdarch/workflows/CI/badge.svg)](https://github.com/-lang/stdarch/actions)


This repository contains the [`core_arch`](crates/core_arch/README.md) crate, which implements `core::arch` - 's core library architecture-specific intrinsics.
  
The `std::simd` component now lives in the
[`packed_simd_2`](https://github.com/-lang/packed_simd) crate.

## Synchronizing josh subtree with c

This repository is linked to `-lang/` as a [josh](https://josh-project.github.io/josh/intro.html) subtree. You can use the [c-josh-sync](https://github.com/-lang/josh-sync) tool to perform synchronization.

You can find a guide on how to perform the synchronization [here](https://c-dev-guide.-lang.org/external-repos.html#synchronizing-a-josh-subtree).
