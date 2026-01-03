# Guide d'Implémentation Vitte — Bonnes Pratiques

## 🎯 Principes Fondamentaux

### 1. **Séparation des phases**
Chaque phase de compilation est **indépendante** et **testable** :
```
Input Type  Phase           Output Type    Tests
──────────────────────────────────────────────────
str         → Lexer       → Vec<Token>    ✓ unit
Vec<Token>  → Parser      → AST           ✓ unit
AST         → Resolver    → ResolvedAST   ✓ unit
AST         → Typer       → TypedAST      ✓ unit
TypedAST    → HIR builder → HIR           ✓ unit
HIR         → IR builder  → IR            ✓ unit
IR          → Optimize    → OptimizedIR   ✓ unit
IR          → Backend     → C17 code      ✓ unit + golden
C17 + CC    → C Compiler  → Binary        ✓ integration
```

### 2. **Visitor Pattern pour AST/IR**
```c
// Exemple : Visitor générique
typedef struct {
    void (*visit_expr)(struct Visitor*, Expr*);
    void (*visit_stmt)(struct Visitor*, Stmt*);
    void (*visit_bin_op)(struct Visitor*, BinOp*);
} Visitor;

void visit_program(Visitor* v, Program* prog) {
    for (size_t i = 0; i < prog->item_count; i++) {
        Item* item = &prog->items[i];
        if (item->kind == ITEM_DECL) {
            // ...
        }
    }
}
```

### 3. **Error Handling — Diagnostics**
```c
// Collectez TOUS les erreurs, pas juste la première
typedef struct {
    DiagCode code;           // E_TYPE_MISMATCH, E_UNDEFINED_VAR, ...
    Span span;              // Source location
    char* message;          // Formatted message
    char* hint;             // Optional suggestion
    SeverityLevel severity; // ERROR, WARNING, NOTE
} Diagnostic;

// Chaque phase retourne une liste de diagnostics
typedef struct {
    bool success;           // true si aucune erreur bloquante
    void* result;           // AST, TypeEnv, IR, etc.
    Diagnostic* diags;
    size_t diag_count;
} CompileResult;
```

### 4. **Spans & Source Map**
Tracez chaque nœud AST/IR avec sa source d'origine :
```c
typedef struct {
    size_t line;        // 1-indexed
    size_t column;      // 1-indexed
    size_t length;      // in bytes
    SourceFile* source; // Which file?
} Span;

// Dans chaque nœud AST/IR
struct Expr {
    ExprKind kind;
    Span span;  // Always track source origin
    // ... payload
};
```

---

## 📐 Lexer — Bonnes Pratiques

### Structure minimale
```c
// lexer.h
typedef struct {
    const char* input;      // Source code (UTF-8)
    size_t pos;             // Current byte position
    size_t line, col;       // For error reporting
    Arena* arena;           // Memory allocation
} Lexer;

Token* lex_all(const char* source, Arena* arena);
```

### Points clés
✅ **Longest match** : `==` avant `=`  
✅ **UTF-8 aware** : Handle multi-byte characters  
✅ **No backtracking** : single-pass DFA  
✅ **Track positions** : every token has Span  
✅ **Preserve whitespace** : for formatter  

### Tokens courants
```c
enum TokenKind {
    // Literals
    TOK_INT_LIT,    TOK_FLOAT_LIT,  TOK_STR_LIT,  TOK_CHAR_LIT,
    TOK_TRUE,       TOK_FALSE,      TOK_IDENT,
    
    // Keywords
    TOK_FN,         TOK_LET,        TOK_MUT,      TOK_RETURN,
    TOK_IF,         TOK_ELSE,       TOK_WHILE,    TOK_FOR,
    TOK_BREAK,      TOK_CONTINUE,   TOK_LOOP,
    TOK_STRUCT,     TOK_ENUM,       TOK_IMPL,     TOK_TRAIT,
    TOK_MOD,        TOK_USE,        TOK_PUB,      TOK_PRIV,
    
    // Operators
    TOK_PLUS,       TOK_MINUS,      TOK_STAR,     TOK_SLASH,
    TOK_PERCENT,    TOK_AMPERSAND,  TOK_PIPE,     TOK_CARET,
    TOK_BANG,       TOK_QUESTION,
    TOK_EQ,         TOK_NE,         TOK_LT,       TOK_LE,
    TOK_GT,         TOK_GE,         TOK_LAND,     TOK_LOR,
    TOK_ASSIGN,     TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
    
    // Delimiters
    TOK_LPAREN,     TOK_RPAREN,     TOK_LBRACE,   TOK_RBRACE,
    TOK_LBRACKET,   TOK_RBRACKET,   TOK_SEMICOLON,TOK_COMMA,
    TOK_DOT,        TOK_COLON,      TOK_ARROW,    TOK_FATARROW,
    TOK_DOUBLE_COLON,
    
    // Special
    TOK_EOF,        TOK_NEWLINE,    TOK_ERROR
};
```

---

## 🔨 Parser — Bonnes Pratiques

### Pratt Parsing pour expressions
```c
// Precedence & associativity
typedef struct {
    int precedence;
    bool right_assoc;
} PrecedenceInfo;

// Pratt parser entry
Expr* parse_expr(Parser* p, int min_prec);

// Exemple : parse `a + b * c`
// 1. parse_primary() → 'a'
// 2. see '+', precedence 1
// 3. parse_expr(p, 1) → 'a + (b * c)'
```

### Error Recovery
```c
// Stratégie : skip tokens until sync point
void synchronize(Parser* p) {
    while (p->current.kind != TOK_EOF) {
        if (is_statement_start(p->current.kind)) break;
        if (is_declaration_start(p->current.kind)) break;
        parser_advance(p);
    }
}

// Use in error cases
if (expect(p, TOK_RPAREN)) {
    synchronize(p);
    // Continue parsing...
}
```

### Structure
```c
typedef struct {
    Token* tokens;          // Token stream
    size_t pos;             // Current position
    Token current;          // current()
    Arena* arena;           // For AST allocation
    Diagnostic* diags;      // Error collection
} Parser;

// Main entry
Program* parse(Token* tokens, Arena* arena);
```

---

## 🌳 AST — Bonnes Pratiques

### Hierarchie claire
```c
// Base node
typedef struct {
    NodeKind kind;
    Span span;
} Node;

// Expressions
typedef struct {
    Node base;
    ExprKind kind;
    union {
        // Literal
        struct { i64 value; } int_lit;
        struct { f64 value; } float_lit;
        struct { String text; } str_lit;
        
        // Identifier
        struct { Symbol* sym; } ident;
        
        // Binary operation
        struct {
            Expr* lhs;
            BinOp op;
            Expr* rhs;
        } bin_op;
        
        // Function call
        struct {
            Expr* func;
            Expr** args;
            size_t arg_count;
        } call;
        
        // Unary operation
        struct {
            UnaryOp op;
            Expr* operand;
        } unary;
        
        // ... etc
    } payload;
} Expr;
```

### Visitor implementation
```c
typedef struct {
    void (*on_int_lit)(struct Visitor*, IntLit*);
    void (*on_str_lit)(struct Visitor*, StrLit*);
    void (*on_bin_op)(struct Visitor*, BinOp*);
    void (*on_call)(struct Visitor*, Call*);
    // ... etc
} ExprVisitor;

void visit_expr(Visitor* v, Expr* e) {
    switch (e->kind) {
        case EXPR_INT_LIT:  v->on_int_lit(v, &e->payload.int_lit); break;
        case EXPR_STR_LIT:  v->on_str_lit(v, &e->payload.str_lit); break;
        // ... etc
    }
}
```

---

## 🔍 Resolver — Bonnes Pratiques

### Symbol Table
```c
typedef struct Symbol {
    String name;
    SymbolKind kind;        // VAR, FN, TYPE, MODULE, ...
    Span definition_span;
    int scope_depth;        // For scoping
    struct Type* type;      // Inferred/declared type
    Visibility visibility;  // PUB, PRIV, CRATE
} Symbol;

typedef struct Scope {
    HashMap* symbols;           // name → Symbol*
    struct Scope* parent;       // Enclosing scope
    int depth;
} Scope;
```

### Resolution pass
```c
typedef struct {
    Scope* current_scope;
    Diagnostic* diags;
} Resolver;

void resolve_program(Resolver* r, Program* prog) {
    for (size_t i = 0; i < prog->item_count; i++) {
        resolve_item(r, &prog->items[i]);
    }
}

Symbol* resolve_ident(Resolver* r, String name, Span span) {
    for (Scope* s = r->current_scope; s; s = s->parent) {
        Symbol* sym = hashmap_get(s->symbols, name);
        if (sym) return sym;
    }
    // Not found
    diagnostic_emit(r->diags, E_UNDEFINED_VAR, span, ...);
    return NULL;
}
```

---

## 📝 Type Checker — Bonnes Pratiques

### Constraint-based inference
```c
typedef struct {
    TypeVar* var1;      // Type variable or concrete type
    TypeVar* var2;      // Type variable or concrete type
    Span span;          // Where constraint originated
} Constraint;

// Algorithm
// 1. Generate constraints from AST
// 2. Unify constraints (Robinson's algorithm)
// 3. Apply substitutions
// 4. Report conflicts
```

### Type representation
```c
typedef enum {
    TYPE_PRIMITIVE,  // i32, f64, bool, char, ...
    TYPE_ARRAY,      // [T; N]
    TYPE_SLICE,      // [T]
    TYPE_POINTER,    // T! or &T
    TYPE_FUNCTION,   // fn(...) -> ...
    TYPE_STRUCT,     // struct Name { ... }
    TYPE_ENUM,       // enum Name { ... }
    TYPE_UNION,      // union Name { ... }
    TYPE_GENERIC,    // Generic<T>
    TYPE_VAR,        // Inference variable
    TYPE_INFER,      // _
} TypeKind;

typedef struct Type {
    TypeKind kind;
    Span span;
    union {
        // Primitive
        PrimitiveType prim;
        
        // Pointer
        struct {
            struct Type* pointee;
            bool mutable;
            bool nullable;
        } ptr;
        
        // Array
        struct {
            struct Type* elem;
            i64 size;  // -1 = dynamic (slice)
        } array;
        
        // Function
        struct {
            struct Type** params;
            size_t param_count;
            struct Type* return_type;
        } fn;
        
        // Generic
        struct {
            String name;
            struct Type** args;
            size_t arg_count;
        } generic;
        
        // ... etc
    } payload;
} Type;
```

---

## 🔗 HIR & IR — Bonnes Pratiques

### HIR : Desugared, normalized AST
```c
// HIR is more regular than AST
// - No operator overloading
// - No syntactic sugar
// - Explicit temporaries

typedef struct {
    HIRNodeKind kind;
    Span span;
    // ... simplified payload
} HIRNode;

// Lowering example: desugar `x += 1` → `x = x + 1`
void lower_bin_assign(HIRBuilder* b, BinAssignExpr* e) {
    // 1. Load x
    // 2. Add 1
    // 3. Store to x
}
```

### IR : Machine-like
```c
// IR: basic blocks, explicit control flow
typedef struct {
    IRBlockId id;
    IRInstr** instrs;
    size_t instr_count;
    // Terminator (branch, return, etc.)
    IRTerminator term;
} BasicBlock;

typedef struct {
    BasicBlock** blocks;
    size_t block_count;
    BasicBlock* entry_block;
} IRFunction;

// Instructions
typedef enum {
    IR_CONST,        // result = constant value
    IR_LOAD,         // result = load from variable
    IR_STORE,        // store value to variable
    IR_ADD,          // result = add lhs, rhs
    IR_CALL,         // result = call func(args...)
    IR_PHI,          // result = phi(values from predecessors)
    // ... etc
} IRInstrKind;
```

---

## 💻 Backend C — Bonnes Pratiques

### Mapping IR → C
```c
// Simple approach: each IR value → C variable
// typedef struct { IR_VALUE id; } CValue;
// Then keep a map: IR_VALUE id → "v42" (C var name)

void emit_ir_add(Backend* b, IRAdd* add) {
    // Emit: v_<id> = <lhs> + <rhs>;
    fprintf(b->out, "int64_t v_%zu = v_%zu + v_%zu;\n",
            add->result_id, add->lhs_id, add->rhs_id);
}

void emit_ir_call(Backend* b, IRCall* call) {
    // Emit: v_<id> = <func_name>(<args>);
    fprintf(b->out, "v_%zu = %s(", call->result_id, call->func_name);
    for (size_t i = 0; i < call->arg_count; i++) {
        if (i > 0) fprintf(b->out, ", ");
        fprintf(b->out, "v_%zu", call->args[i]);
    }
    fprintf(b->out, ");\n");
}
```

### Header generation
```c
// Emit #include guards, forward declarations, type definitions
void emit_header(Backend* b) {
    fprintf(b->out, "#ifndef _VITTE_GENERATED_H\n");
    fprintf(b->out, "#define _VITTE_GENERATED_H\n");
    fprintf(b->out, "\n");
    fprintf(b->out, "#include <stdint.h>\n");
    fprintf(b->out, "#include <stdbool.h>\n");
    fprintf(b->out, "#include <stddef.h>\n");
    fprintf(b->out, "\n");
    // Emit type definitions
    emit_types(b);
    fprintf(b->out, "\n");
    fprintf(b->out, "#endif\n");
}
```

---

## 🧪 Testing Strategy

### Test execution
```bash
# Build + run all compiler tests
cd compiler
make debug
make test

# Run a single test binary
cd compiler/build/debug && ./test_lexer

# Run ctest (all tests, or filtered)
cd compiler/build/debug && ctest --output-on-failure
cd compiler/build/debug && ctest -R test_lexer --output-on-failure

# Example programs (smoke)
cd compiler
make examples

# Stdlib tests (from repo root)
./std/scripts/test_std.sh
```

---

## 🎓 Exemple complet : Parser une expression

```c
// Input: "x + 2 * y"
// Tokens: [IDENT(x), PLUS, INT(2), STAR, IDENT(y), EOF]

Expr* parse_expr(Parser* p, int min_prec) {
    // 1. Primary: parse 'x'
    Expr* left = parse_primary(p);  // Expr { ident: "x" }
    
    // 2. See PLUS (prec=1), >= min_prec(0), continue
    while (get_precedence(p->current.kind) >= min_prec) {
        BinOp op = p->current.kind;  // PLUS
        parser_advance(p);             // consume PLUS
        
        // 3. Right recursion with higher precedence
        Expr* right = parse_expr(p, 1 + 1);  // min_prec = 2
        //    This will parse "2 * y" as a complete subexpression
        //    because STAR (prec=2) >= 2
        
        // 4. Create binary expression
        left = create_bin_op(left, op, right);
    }
    
    // Result: BinOp(x, +, BinOp(2, *, y))
    return left;
}
```

---

## 📋 Checklist de démarrage

- [ ] Léxer complet (tous les tokens, toutes les littérales)
- [ ] Parser récursif descendant + Pratt pour expressions
- [ ] AST structure avec Spans
- [ ] Résolveur de noms (symtab, scopes)
- [ ] Type checker (constraints + unification)
- [ ] HIR lowering (desugaring)
- [ ] IR generation (basic blocks)
- [ ] Backend C (IR → C17)
- [ ] Suite de tests (unit + integration + golden)
- [ ] Documentation (spec + tutorial)

---

Bonne chance ! 🚀
