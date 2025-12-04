# MVP front‑end + IR + bytecode (Vitte)

Objectif : livrer une première boucle compiler → bytecode → VM sur un sous‑ensemble stable du langage, en s’alignant sur `docs/grammar.ebnf.md`, `grammar/vitte.pest` et `docs/bytecode-spec.md`.

## 1. Sous-ensemble syntaxe (parser MVP)
- Modules : `module foo.bar`
- Fonctions : `fn name(param: Type, ...) -> Type: <block>.end`
- Déclarations locales : `let x = expr`, `const X = expr`
- Instructions : expression nue, `return expr?`, `if cond: ... .end`, `while cond: ... .end`
- Expressions : identifiants, littéraux int/float/bool/string, appel `f(a, b)`, parenthèses, opérateurs binaires `+ - * / % == != < <= > >= and or`, unaire `-`, `not`
- Pas de types complexes ni génériques pour la boucle MVP (type `Any`/`Unknown` par défaut).

## 2. Lexer minimal
- Tokens nécessaires : identifiant, nombre (int/float), string, mots-clés (`fn`, `let`, `const`, `if`, `else`, `while`, `return`, `module`), ponctuation `() , : -> .end`, opérateurs `+ - * / % == != < <= > >= and or not`.
- Indentation : garder l’option `emit_indent_tokens=true` pour conserver la structure Python‑like, mais le parser MVP peut aussi fonctionner en `.end` explicite (tolérer les deux).
- Fichier de référence : `compiler/src/l/compiler/lexer.vitte` (réutiliser les helpers existants, limiter la table de mots-clés à ceux du sous-ensemble).

## 3. AST léger à cibler
Structures cibles (soit dans `ast.vitte`, soit un AST « mini » dédié) :
- `Program { module_name: String, functions: Vec<Function> }`
- `Function { name: String, params: Vec<Param>, ret: Type?, body: Block }`
- `Block { stmts: Vec<Stmt> }`
- `Stmt = Let(name, Expr) | Const(name, Expr) | ExprStmt(Expr) | Return(Expr?) | If(cond, Block, else_block?) | While(cond, Block)`
- `Expr = Literal | Name | Call(callee, args) | Unary(op, Expr) | Binary(op, lhs, rhs) | Paren(Expr)`
- `Type` peut être un enum minimal (`Unknown`, `Bool`, `I64`, `F64`, `Unit`) pour amorcer le typage.

## 4. Parser MVP (descente récursive)
- Entrée : tokens du lexer (en sautant `NEWLINE/INDENT/DEDENT` trivia si `.end` utilisé).
- Règles principales :
  - `program := module_decl? top_level* EOF`
  - `top_level := function_decl | stmt` (les `let/const` globaux peuvent être rejetés pour l’instant)
  - `function_decl := "fn" ident "(" params? ")" ("->" type)? ":" block ".end"`
  - `block := stmt*` jusqu’à `.end`
  - `stmt := let | const | return | if | while | expr`
  - `expr` : opérateurs avec précédences (unaires, multiplicatifs, additifs, comparaisons, and/or).
- Fichier de référence : `compiler/src/l/compiler/parser.vitte` (réduire l’ensemble des branches aux constructions ci‑dessus).

## 5. IR minimal (stable et simple)
Valeurs : SSA light avec registres numérotés.
- `IrType = Unit | Bool | I64 | F64 | Unknown`
- `IrValueId` (index), `IrBlockId`, `IrFuncId`
- `IrInstr = ConstInt(i64) | ConstFloat(f64) | ConstBool(Bool) | Copy(src) | Bin(op, lhs, rhs) | Cmp(op, lhs, rhs) | Jump(target) | JumpIf(cond, then, else) | Call(func, args) | Return(value?)`
- `IrBlock { id, instrs, term }` (terminateur = Jump/JumpIf/Return)
- `IrFunction { params: Vec<(name, ty)>, locals: Map<String, IrValueId>, blocks }`
- `IrModule { functions }`
- Fichier cible : `compiler/src/l/compiler/ir_builder.vitte` (remplacer les NOP par ces instrs, sans SSA stricte).

## 6. Table des symboles (scopes)
- Stack de scopes `{name -> IrValueId}`.
- Lors de l’entrée dans un block, pousser un scope ; le pop à la sortie.
- Les fonctions capturant des variables ne sont pas supportées au MVP ; si une capture est rencontrée, lever une erreur `Unsupported capture`.

## 7. Bytecode MVP (mapping vers `docs/bytecode-spec.md`)
- Utiliser le pool de constantes pour int/float/bool/string (tags 0–4).
- Instructions à émettre :
  - `CONST k` (0x10), `LOAD_LOCAL a` (0x11), `STORE_LOCAL a` (0x12)
  - `ADD/SUB/MUL/DIV/MOD` (0x22..0x26)
  - `EQ/NE/LT/LE/GT/GE` (0x30..0x35)
  - `JUMP` (0x40), `JUMP_IF_TRUE` (0x41), `JUMP_IF_FALSE` (0x42)
  - `CALL argc` (0x50), `RET` (0x52)
- Forme stack-based : l’IR `Bin(op, a, b)` se traduit par `LOAD a`, `LOAD b`, `<OP>`.
- `FunctionEntry` : `param_count = params.len`, `local_count = locals.len`, `max_stack` calculé naïvement (profondeur max des emplacements poussés).
- Fichier cible pour l’émetteur : `compiler/src/l/compiler/codegen_lvm.vitte` ou un nouveau `compiler/src/l/compiler/bytecode_emit.vitte` simplifié.

## 8. Boucle de compilation cible
1. `lex_source` → tokens
2. `parse_source` → AST mini
3. `build_ir_module` → IR minimal
4. `emit_bytecode` → `{constants, functions, code}` struct logique (pas d’I/O)
5. Exécution avec `vm.vitte` en important ce chunk logique

## 9. Étapes concrètes recommandées
1. Réduire/dupliquer `lexer.vitte` en mode minimal : garder uniquement les mots-clés du sous-ensemble.
2. Implémenter le parser récursif pour le sous-ensemble dans `parser.vitte` (ou `parser_mini.vitte`) en s’appuyant sur les `Span` existants pour les erreurs.
3. Remplacer les NOP du `ir_builder.vitte` par les instrs décrites en §5 pour `let`, `return`, `if/while`, binaires et appels.
4. Ajouter un petit émetteur bytecode qui mappe l’IR aux opcodes §7 et produit une structure `BytecodeChunk`.
5. Brancher `pipeline.vitte` : stop_after=IR/Codegen pour renvoyer le chunk logique ; ajouter un test d’intégration (ex: fonction `add(a,b)` et un `main` qui retourne 42).
