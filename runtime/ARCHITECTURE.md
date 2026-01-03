# Vitte Runtime Architecture

## Vue d'ensemble

Le système runtime Vitte est une machine virtuelle complète avec:
- **Type system**: 12 types primitifs + structures dynamiques
- **Execution engine**: Stack + Registers, 33 opcodes
- **Memory management**: Heap 10MB avec garbage collection Mark-Sweep
- **Built-ins**: 16 fonctions natives intégrées
- **Debugging**: Disassembler, traçage, breakpoints
- **ABI**: Conventions d'appel standardisées

## Modules Principaux

### 1. Type System (vitte_types.*)
- **Union étiquetée**: `vitte_value_t` = type tag + union données
- **Polymorphisme**: Conversion auto int/float en opérations arithmétiques
- **Collections**: Arrays, Strings avec gestion mémoire
- **OOP**: Classes, Instances, Closures

### 2. Virtual Machine (vitte_vm.*)
- **Chunk bytecode**: Conteneur code + constantes
- **Execution loop**: Interpréteur bytecode principal
- **Stack machine**: 256 éléments opérandes
- **Register machine**: 16 registres pour stockage local
- **Heap allocation**: Allocation dynamique sur tas 10MB

### 3. Garbage Collector (garbage_collector.*)
- **Mark-Sweep**: Deux phases marquer puis balayer
- **Reachability analysis**: Marque depuis stack/registers/globals
- **Adaptive threshold**: Déclenche GC à 2x allocation précédente
- **Statistics**: Suivi objets collectés, bytes, runs

### 4. Built-in Functions (builtin.*)
Fonctions natives exposées:
- **Strings**: strlen, substr, strcat, strtoupper
- **Math**: sqrt, abs, floor, ceil, pow
- **Arrays**: array_len, array_push, array_pop
- **Type conversions**: to_int, to_float, to_string, type_of

### 5. Calling Convention (calling_convention.*)
- **6 argument registers**: ARG0-ARG5
- **Stack frames**: Sauvegarde contexte appel
- **Variadic support**: Fonctions variables

### 6. Memory Management (memory.*)
- **Allocation tracking**: Liste tous les blocs alloués
- **Statistics**: Total alloué, pic usage, blocs actifs
- **Leak detection**: Dump blocks pour analyse

### 7. Debugger (debug.*)
- **Instruction tracing**: Log chaque instruction exécutée
- **Stack tracing**: Monitorage pile d'exécution
- **Memory tracing**: Suivi allocations/dé-allocations
- **Breakpoints**: Points d'arrêt par ligne/fonction
- **Statistics**: Compteurs opérations

### 8. Disassembler (disassembler.*)
- **Bytecode display**: Affichage lisible du code machine
- **Constant listing**: Liste constantes avec affichage
- **Opcode details**: Noms, descriptions, opérandes
- **Colored output**: Syntaxe couleur optionnelle

## Flux d'Exécution

```
1. Runtime Init
   └─> Créer VM
   └─> Créer GC (1MB threshold)
   └─> Créer Memory Manager
   └─> Créer Debug Context

2. Compilation/Chargement
   └─> Parser bytecode source
   └─> Créer chunk bytecode
   └─> Ajouter constantes

3. Exécution
   └─> vitte_vm_execute(chunk)
   └─> Interpreter loop
       ├─> Read opcode
       ├─> Execute operation
       ├─> Update stack/registers
       └─> Check errors
   └─> Return result

4. Cleanup
   └─> GC sweep
   └─> Free memory
   └─> Close VM
```

## Structures Données Clés

### vitte_value_t
```c
typedef struct {
    vitte_type_t type;          // Tag type
    union {
        bool boolean;
        int64_t integer;
        double floating;
        void *pointer;
        vitte_string_t string;
    } as;
} vitte_value_t;
```

### vitte_vm_t
```c
typedef struct {
    vitte_value_t stack[256];        // Stack
    vitte_value_t registers[16];     // Registers
    uint8_t *heap;                   // Heap 10MB
    
    uint8_t *ip;                     // Instruction pointer
    chunk_t *chunk;                  // Bytecode courant
    void *globals;                   // Global variables
    
    call_frame_t frames[64];         // Call stack
    int frame_count;
    
    char error_buffer[512];
    bool had_error;
} vitte_vm_t;
```

### chunk_t
```c
typedef struct {
    uint8_t *code;                   // Bytecode
    vitte_value_t *constants;        // Literals
    int *lines;                      // Debug line info
    
    int code_size;
    int code_capacity;
    int constant_count;
    int constant_capacity;
} chunk_t;
```

## Opcodes (33 Total)

| Catégorie | Opcodes |
|-----------|---------|
| Stack | CONST, NIL, TRUE, FALSE, POP |
| Variables | GET_LOCAL, SET_LOCAL, GET_GLOBAL, SET_GLOBAL |
| Comparaison | EQUAL, GREATER, LESS |
| Arithmétique | ADD, SUBTRACT, MULTIPLY, DIVIDE, NOT, NEGATE |
| I/O | PRINT |
| Contrôle | JUMP, JUMP_IF_FALSE, LOOP |
| Fonctions | CALL, RETURN |
| OOP | CLASS, METHOD, INVOKE |
| Collections | ARRAY, INDEX_GET, INDEX_SET |
| Registres | LOAD_REG, STORE_REG |
| Système | HALT |

## Performance

- **Stack ops**: O(1) push/pop
- **Register access**: O(1) load/store
- **Global lookup**: O(1) avec hashtable
- **GC Mark**: O(n) n=objets marqués
- **GC Sweep**: O(n) n=objets total

## Sécurité

- **Bounds checking**: Validation stack/register access
- **Type safety**: Dispatch checked par type tag
- **Memory safety**: Tracking allocation suivi
- **GC safety**: Reachability-based sweep

## Extension Points

1. **New types**: Ajouter vitte_type_t enum value
2. **New opcodes**: Ajouter case dans interpreter loop
3. **Built-in functions**: Enregistrer dans builtin_registry
4. **GC policy**: Modifier threshold adaptatif
5. **ABI**: Ajuster register allocation scheme

## Limitations Actuelles

- GC stop-the-world (pas concurrent)
- Stack/heap taille fixe
- Pas JIT compilation
- Pas optimisation bytecode
- Pas string interning
