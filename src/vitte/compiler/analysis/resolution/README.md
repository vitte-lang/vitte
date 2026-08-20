# Vitte Compiler — Analysis / Resolution

The `analysis/resolution` directory contains the complete semantic resolution subsystem of the Vitte compiler.

Its purpose is to transform unresolved names, paths, modules, namespaces, imports, exports, constants, fields, methods, generic parameters, traits, attributes and overloaded declarations into stable compiler entities that can be consumed by semantic analysis, type checking and later compiler stages.

Resolution is positioned after syntactic analysis and structural lowering, and before the compiler requires complete semantic knowledge of the program.

```text

source

  |

  v

lexer

  |

  v

parser

  |

  v

AST

  |

  v

HIR

  |

  v

analysis

  |

  +-- modules

  +-- scopes

  +-- symbols

  +-- resolution

        |

        +-- names

        +-- modules

        +-- namespaces

        +-- paths

        +-- imports

        +-- exports

        +-- constants

        +-- attributes

        +-- fields

        +-- methods

        +-- generics

        +-- traits

        +-- overloads

        +-- visibility

        |

        v

resolved semantic references

  |

  v

sema

  |

  v

typeck

  |

  v

borrowck

  |

  v

MIR

  |

  v

IR

  |

  v

backend
```



The subsystem is designed around deterministic resolution, explicit context, stable identifiers, reusable lookup operations, dependency tracking, cycle detection, caching, diagnostics and validation.

```text
src/vitte/compiler/analysis/resolution/
├── attribute_resolution.vit
├── candidate.vit
├── constant_resolution.vit
├── export_resolution.vit
├── field_resolution.vit
├── generic_resolution.vit
├── import_resolution.vit
├── lookup_cache.vit
├── lookup.vit
├── method_resolution.vit
├── module_resolution.vit
├── name_resolution.vit
├── namespace_resolution.vit
├── overload_resolution.vit
├── path_resolution.vit
├── resolution_context.vit
├── resolution_pass.vit
├── resolution_result.vit
├── resolution.vit
├── resolver_diagnostics.vit
├── resolver_graph.vit
├── resolver_stack.vit
├── resolver_state.vit
├── resolver_statistics.vit
├── resolver.vit
├── scope_resolution.vit
├── symbol_resolution.vit
├── trait_resolution.vit
├── validation.vit
├── visibility_resolution.vit
└── README.md
```

## Responsibilities

### The resolution layer is responsible for:

* resolving identifiers;
* resolving lexical names;
* resolving symbols;
* resolving scopes;
* resolving modules;
* resolving namespaces;
* resolving qualified paths;
* resolving imports;
* resolving exports;
* resolving constants;
* resolving attributes;
* resolving fields;
* resolving methods;
* resolving generic parameters;
* resolving trait references and implementations;
* constructing overload candidate sets;
* selecting overload candidates;
* checking visibility during lookup;
* tracking active resolution operations;
* detecting recursive resolution;
* detecting dependency cycles;
* caching reusable lookup results;
* invalidating stale cached results;
* recording resolution statistics;
* generating structured resolver diagnostics;
* validating final resolution results.

### The subsystem does not own:

* lexical analysis;
* parsing;
* AST construction;
* complete HIR lowering;
* full type inference;
* ownership checking;
* borrow checking;
* MIR construction;
* optimization;
* machine code generation;
* object emission;
* linking.

Those responsibilities belong to other compiler layers.

### Core architecture

```text

                         resolution.vit
                               |
                               v
                          resolver.vit
                               |
          +--------------------+--------------------+
          |                    |                    |
          v                    v                    v
 ResolutionContext       ResolverState            Lookup
          |                    |                    |
          |              +-----+------+             |
          |              |            |             |
          |              v            v             v
          |       ResolverStack  ResolverGraph  LookupCache
          |                                         |
          +---------------------+-------------------+
                                |
                                v
                            Candidate
                                |
       +------------------------+-------------------------+
       |                        |                         |
       v                        v                         v
 lexical/module            member/type             package/API
 resolution                resolution              resolution
       |                        |                         |
       + name                  + field                   + import
       + scope                 + method                  + export
       + symbol                + generic
       + module                + trait
       + namespace             + overload
       + path
       |
       +------------------------+-------------------------+
                                |
                                v
                        ResolutionResult
                                |
                    +-----------+-----------+
                    |                       |
                    v                       v
               Validation
```
