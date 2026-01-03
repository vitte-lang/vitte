# Vitte Runtime System

Un système d'exécution complet pour la machine virtuelle Vitte, construisant un environnement d'exécution bytecode haute performance avec gestion de mémoire et garbage collection.

## Architecture

### Composants Principaux

#### 1. **Système de Types (`include/vitte_types.h` + `src/vitte_types.c`)**
- **12 Types de valeurs**: NIL, BOOL, INT, FLOAT, STRING, ARRAY, OBJECT, FUNCTION, NATIVE_FUNCTION, CLOSURE, CLASS, INSTANCE
- **Union étiquetée**: Représentation mémoire efficace avec dispatch par type
- **Opérations**: Création, vérification de type, arithmétique, comparaison
- **Totalement polymorphe**: Int/Float auto-conversion en opérations arithmétiques

#### 2. **Machine Virtuelle (`include/vitte_vm.h` + `src/vitte_vm.c`)**
- **33 Opcodes**: Complètes opérations bytecode
- **Architecture Stack+Register**: 
  - Stack: 256 éléments pour opérations
  - Registers: 16 registres pour stockage local
  - Heap: 10MB mémoire dynamique
- **Interpreter Loop**: Exécution bytecode optimisée
- **Gestion d'erreurs**: Buffer erreur, codes résultat

#### 3. **Garbage Collector (`gc/garbage_collector.h/c`)**
- **Mark-Sweep GC**: Algorithme stop-the-world
- **Tracking objets**: Liste objets alloués avec marquage
- **Threshold adaptatif**: Déclenche GC à 2x allocation précédente
- **Statistiques**: Suivi runs GC, objets collectés, bytes alloués

#### 4. **Fonctions Intégrées (`src/builtin.h/c`)**
16 fonctions natives:
- **String**: strlen, substr, strcat, strtoupper
- **Math**: sqrt, abs, floor, ceil, pow
- **Array**: array_len, array_push, array_pop
- **Type**: to_int, to_float, to_string, type_of

#### 5. **Conventions d'Appel (`abi/calling_convention.h/c`)**
- **6 registres arguments**: ARG0-ARG5
- **Stack frame**: Structure organisation pile
- **Variadic support**: Fonctions avec arguments variables

#### 6. **Runtime Principal (`src/main.c`)**
- **REPL interactif**: Boucle lecture-évaluation-affichage
- **Test suite**: Programmes test arithmétique, strings, arrays
- **Monitoring**: Stats GC, info mémoire

## Structure des Fichiers

```
runtime/
├── include/
│   ├── vitte_types.h      (102 LOC) - Système de types
│   └── vitte_vm.h         (225 LOC) - Architecture VM
├── src/
│   ├── vitte_types.c      (242 LOC) - Implémentation types
│   ├── vitte_vm.c         (410 LOC) - Interpréteur bytecode
│   ├── builtin.c          (285 LOC) - Fonctions intégrées
│   ├── builtin.h          (42 LOC)  - API builtins
│   └── main.c             (295 LOC) - Entry point + REPL
├── gc/
│   ├── garbage_collector.h (41 LOC) - API GC
│   └── garbage_collector.c (193 LOC) - Implémentation GC
├── abi/
│   ├── calling_convention.h (48 LOC) - Conventions appel
│   └── calling_convention.c (130 LOC) - Gestion call frames
├── CMakeLists.txt         - Configuration build
└── README.md             - Documentation
```

## Spécifications Techniques

### Type System
```c
typedef union {
    bool     boolean;
    int64_t  integer;
    double   floating;
    void     *pointer;
    vitte_string_t string;
} vitte_value_as_t;

typedef struct {
    vitte_type_t type;
    vitte_value_as_t as;
} vitte_value_t;
```

### Virtual Machine
```c
typedef struct {
    vitte_value_t stack[256];      // Stack principal
    vitte_value_t registers[16];   // Registres locaux
    uint8_t       *heap;           // Mémoire dynamique (10MB)
    size_t        heap_used;       // Heap utilisé
    
    uint8_t       *ip;             // Instruction pointer
    chunk_t       *chunk;          // Bytecode courant
    void          *globals;        // Variables globales
    
    call_frame_t  frames[64];      // Call stack
    int           frame_count;
    
    uint64_t      instruction_count;
    
    char          error_buffer[512];
    bool          had_error;
} vitte_vm_t;
```

### Opcodes (33 total)
- **Stack**: CONST, NIL, TRUE, FALSE, POP
- **Variables**: GET_LOCAL, SET_LOCAL, GET_GLOBAL, SET_GLOBAL
- **Comparaison**: EQUAL, GREATER, LESS
- **Arithmétique**: ADD, SUBTRACT, MULTIPLY, DIVIDE, NOT, NEGATE
- **I/O**: PRINT
- **Contrôle**: JUMP, JUMP_IF_FALSE, LOOP
- **Fonctions**: CALL, RETURN
- **OOP**: CLASS, METHOD, INVOKE
- **Collections**: ARRAY, INDEX_GET, INDEX_SET
- **Registres**: LOAD_REG, STORE_REG
- **Système**: HALT

## Compilation et Utilisation

### Build
```bash
cd runtime
mkdir build
cd build
cmake ..
cmake --build .
```

### Exécution REPL
```bash
./build/vitte-runtime
```

Commandes REPL:
- `help` - Affiche aide
- `builtins` - Liste fonctions natives
- `gc_stats` - Statistiques garbage collector
- `mem_info` - Info mémoire
- `exit` - Quitter

### Tests
```bash
./build/vitte-runtime --test
```

## Exemple d'Utilisation

```c
// Créer VM
vitte_vm_t *vm = vitte_vm_create();

// Créer bytecode
chunk_t *chunk = chunk_create();
int const_42 = chunk_add_constant(chunk, vitte_int(42));
chunk_write(chunk, OP_CONST, 1);
chunk_write(chunk, const_42, 1);
chunk_write(chunk, OP_PRINT, 1);
chunk_write(chunk, OP_HALT, 1);

// Exécuter
vitte_vm_execute(vm, chunk);

// Cleanup
chunk_free(chunk);
vitte_vm_free(vm);
```

## Performance

- **Stack operations**: O(1)
- **Register access**: O(1)
- **Variable lookup**: O(1) globals avec hashtable
- **GC Mark-Sweep**: O(n) où n = objets alloués
- **Heap allocation**: Linéaire jusqu'à seuil GC

## Limitations Actuelles

- GC stop-the-world simple (pas incremental)
- Pas de JIT compilation
- Pas de optimisation bytecode
- Stack taille fixe (256 elements)
- Heap fixe (10MB)

## Améliorations Futures

- [ ] GC incremental / generationnel
- [ ] JIT compilation x86-64
- [ ] Stack dynamique
- [ ] Heap resizable
- [ ] Call stack optimisée
- [ ] Inline caching
- [ ] String interning
- [ ] FFI C complet
