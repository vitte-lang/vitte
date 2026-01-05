# vitte_coco

Type Constraints and Constraint Solving for Vitte.

## Features

- Constraint types (Equal, Subtype, Satisfies)
- Type representation (Primitive, Generic, Function, Unknown)
- Trait definition and constraint satisfaction
- Constraint set management
- Constraint context with variable binding
- Unification engine for type solving
- ConstraintSolver trait for extensibility

## Usage

```vit
use vitte_coco::{ConstraintSet, Constraint, Type, ConstraintContext, UnificationEngine};

fn main() {
    let mut cs = ConstraintSet::new();
    let c = Constraint::Equal(
        Type::Primitive("i32".to_string()),
        Type::Primitive("i32".to_string())
    );
    cs.add(c);
    
    let mut engine = UnificationEngine::new();
    let result = engine.unify(Type::Primitive("i32".to_string()), Type::Primitive("i32".to_string()));
}
```
