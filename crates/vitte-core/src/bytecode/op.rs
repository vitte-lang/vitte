//! op.rs — Définition des opcodes de la VM Vitte.
//!
//! Design goals :
//! - **Lisible** (bons noms, `Debug`/`Display` propres)
//! - **Extensible** (ajouter des variantes *à la fin* sans casser les ID)
//! - **Pratique** : helpers `mnemonic()`, `stack_delta()`, `is_jump()`,
//!   `jump_target(pc)`, `is_terminator()`…
//!
//! ⚠️ Compat : l’encodage bincode des enums dépend **de l’ordre** des variantes.
//! Si tu ajoutes un opcode, **ajoute-le en bas**. Si tu modifies la sémantique
//! ou la structure globale du chunk, incrémente `CHUNK_VERSION` dans `chunk.rs`.

#![allow(clippy::manual_strip)] // pour les affichages “mnemonic + operands”

// Sérialisation optionnelle (gardée légère dans vitte-core)
#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Registres/arguments usuels (aliases sémantiques)
pub type Reg       = u8;
pub type ConstIx   = u32;
pub type UpvalueIx = u16;
pub type LocalIx   = u16;
pub type FuncIx    = u32;

/// Jeu d’instructions MVP.
///
/// **Compat bincode : ne pas réordonner. Ajouter les nouvelles variantes en bas.**
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Op {
    // ----- Structure -----
    #[default]
    Nop,                        // no-op
    Return,                     // retourne la valeur au top (convention VM)
    ReturnVoid,                 // retourne () (aucune valeur)

    // ----- Constantes & littéraux -----
    LoadConst(ConstIx),         // push const[idx]
    LoadTrue,                   // push true
    LoadFalse,                  // push false
    LoadNull,                   // push null

    // ----- Locals -----
    LoadLocal(LocalIx),         // push locals[idx]
    StoreLocal(LocalIx),        // pop  -> locals[idx]

    // ----- Arith / logique -----
    Add, Sub, Mul, Div, Mod,    // binaires (num)      delta = -1
    Neg,                        // unaire  (num)       delta =  0
    Not,                        // unaire  (bool)      delta =  0

    // Comparaisons → bool
    Eq, Ne, Lt, Le, Gt, Ge,     // binaires            delta = -1

    // ----- Contrôle -----
    Jump(i32),                  // pc = pc + 1 + off
    JumpIfFalse(i32),           // if !cond { jump } (consommation cond = impl VM)
    Pop,                        // drop top

    // ----- Appels -----
    Call(u8),                   // argc ; convention: [..., callee, a0..aN-1] -> result
    TailCall(u8),               // idem, avec sémantique "tail"

    // ----- Debug / I/O -----
    Print,                      // pop et affiche (dev)

    // ----- Fermetures (future) -----
    MakeClosure(FuncIx, u8),    // capture N upvalues
    LoadUpvalue(UpvalueIx),
    StoreUpvalue(UpvalueIx),

    // ----- (Extensions futures — AJOUTER EN BAS) -----
    // Dup, Swap, Halt, Assert, …
}

/// Limites/constantes usuelles
pub const MAX_ARGC: u8 = u8::MAX;

impl Op {
    /// Mnémonique court (pour assembleur, logs, messages d’erreur).
    pub fn mnemonic(&self) -> &'static str {
        use Op::*;
        match *self {
            Nop             => "nop",
            Return          => "ret",
            ReturnVoid      => "retv",
            LoadConst(_)    => "ldc",
            LoadTrue        => "ldtrue",
            LoadFalse       => "ldfalse",
            LoadNull        => "ldnull",
            LoadLocal(_)    => "ldl",
            StoreLocal(_)   => "stl",
            Add             => "add",
            Sub             => "sub",
            Mul             => "mul",
            Div             => "div",
            Mod             => "mod",
            Neg             => "neg",
            Not             => "not",
            Eq              => "eq",
            Ne              => "ne",
            Lt              => "lt",
            Le              => "le",
            Gt              => "gt",
            Ge              => "ge",
            Jump(_)         => "jmp",
            JumpIfFalse(_)  => "jz",
            Pop             => "pop",
            Call(_)         => "call",
            TailCall(_)     => "tcall",
            Print           => "print",
            MakeClosure(_, _)=> "mkclo",
            LoadUpvalue(_)  => "ldu",
            StoreUpvalue(_) => "stu",
        }
    }

    /// Variation **théorique** de la profondeur de pile (delta).
    ///
    /// `Some(Δ)` si statique ; `None` si dépend du runtime (ex: `Call`).
    pub fn stack_delta(&self) -> Option<i32> {
        use Op::*;
        match *self {
            Nop | Jump(_) | JumpIfFalse(_)        => Some(0),
            LoadConst(_) | LoadTrue | LoadFalse | LoadNull
                                                  => Some(1),
            LoadLocal(_)                          => Some(1),
            StoreLocal(_) | Pop | Print           => Some(-1),
            Add|Sub|Mul|Div|Mod|Eq|Ne|Lt|Le|Gt|Ge => Some(-1), // 2 -> 1
            Neg|Not                               => Some(0),  // 1 -> 1
            LoadUpvalue(_)                        => Some(1),
            StoreUpvalue(_)                       => Some(-1),
            MakeClosure(_, _)                     => Some(1),
            Return | ReturnVoid                   => None,     // quitte la frame
            Call(_) | TailCall(_)                 => None,     // dépend de la conv d'appel
        }
    }

    /// L’instruction est-elle un saut (pc modifié) ?
    pub fn is_jump(&self) -> bool {
        matches!(self, Op::Jump(_) | Op::JumpIfFalse(_))
    }

    /// Retourne l’offset relatif si `self` est un saut.
    pub fn jump_offset(&self) -> Option<i32> {
        match *self {
            Op::Jump(ofs) | Op::JumpIfFalse(ofs) => Some(ofs),
            _ => None,
        }
    }

    /// Calcule la **destination** du saut (si valide), en prenant `pc` courant.
    ///
    /// Rappel : l’offset est relatif à `pc+1`, donc `dest = pc + 1 + off`.
    pub fn jump_target(&self, pc: u32) -> Option<u32> {
        self.jump_offset().map(|ofs| {
            let next = pc as i64 + 1;
            let dest = next + ofs as i64;
            if dest < 0 { 0 } else { dest as u32 }
        })
    }

    /// Est-ce un **terminateur** de bloc/trace ? (utile pour analyse/optimisation)
    pub fn is_terminator(&self) -> bool {
        matches!(self, Op::Return | Op::ReturnVoid)
    }

    /// Spécifie si l’opcode embarque un **opérande immédiat** (et de quel type).
    pub fn operand_kind(&self) -> OperandKind {
        use Op::*;
        match *self {
            LoadConst(_)     => OperandKind::Const,
            LoadLocal(_)     => OperandKind::Local,
            StoreLocal(_)    => OperandKind::Local,
            LoadUpvalue(_)   => OperandKind::Upvalue,
            StoreUpvalue(_)  => OperandKind::Upvalue,
            Jump(_)          => OperandKind::RelOffset,
            JumpIfFalse(_)   => OperandKind::RelOffset,
            Call(_)          => OperandKind::Argc,
            TailCall(_)      => OperandKind::Argc,
            MakeClosure(_, n)=> OperandKind::FuncPlusN(n),
            _                => OperandKind::None,
        }
    }
}

/// Nature de l’opérande immédiat d’un opcode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum OperandKind {
    None,
    Const,
    Local,
    Upvalue,
    Argc,
    RelOffset,
    /// Cas particulier : `MakeClosure(func_ix, n_upvalues)`
    FuncPlusN(u8),
}

/* -------------------------- Affichage lisible -------------------------- */

impl core::fmt::Display for Op {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        use Op::*;
        match *self {
            Nop              => write!(f, "nop"),
            Return           => write!(f, "ret"),
            ReturnVoid       => write!(f, "retv"),
            LoadConst(ix)    => write!(f, "ldc {}", ix),
            LoadTrue         => write!(f, "ldtrue"),
            LoadFalse        => write!(f, "ldfalse"),
            LoadNull         => write!(f, "ldnull"),
            LoadLocal(ix)    => write!(f, "ldl {}", ix),
            StoreLocal(ix)   => write!(f, "stl {}", ix),
            Add              => write!(f, "add"),
            Sub              => write!(f, "sub"),
            Mul              => write!(f, "mul"),
            Div              => write!(f, "div"),
            Mod              => write!(f, "mod"),
            Neg              => write!(f, "neg"),
            Not              => write!(f, "not"),
            Eq               => write!(f, "eq"),
            Ne               => write!(f, "ne"),
            Lt               => write!(f, "lt"),
            Le               => write!(f, "le"),
            Gt               => write!(f, "gt"),
            Ge               => write!(f, "ge"),
            Jump(ofs)        => write!(f, "jmp {:+}", ofs),
            JumpIfFalse(ofs) => write!(f, "jz {:+}", ofs),
            Pop              => write!(f, "pop"),
            Call(argc)       => write!(f, "call {}", argc),
            TailCall(argc)   => write!(f, "tcall {}", argc),
            Print            => write!(f, "print"),
            MakeClosure(fi,n)=> write!(f, "mkclo {} {}", fi, n),
            LoadUpvalue(ix)  => write!(f, "ldu {}", ix),
            StoreUpvalue(ix) => write!(f, "stu {}", ix),
        }
    }
}

/* ------------------------------- Tests ------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mnemonics_are_stable() {
        assert_eq!(Op::Add.mnemonic(), "add");
        assert_eq!(Op::LoadTrue.mnemonic(), "ldtrue");
        assert_eq!(Op::Jump(0).mnemonic(), "jmp");
        assert_eq!(Op::TailCall(3).mnemonic(), "tcall");
    }

    #[test]
    fn stack_deltas_basic() {
        assert_eq!(Op::LoadConst(0).stack_delta(), Some(1));
        assert_eq!(Op::Add.stack_delta(), Some(-1));
        assert_eq!(Op::Neg.stack_delta(), Some(0));
        assert_eq!(Op::Pop.stack_delta(), Some(-1));
        assert_eq!(Op::Return.stack_delta(), None);
        assert_eq!(Op::Call(2).stack_delta(), None);
    }

    #[test]
    fn jump_math() {
        let j = Op::Jump(-2);
        assert_eq!(j.jump_offset(), Some(-2));
        assert_eq!(j.jump_target(10), Some(9)); // 10+1-2 = 9
        let jz = Op::JumpIfFalse(5);
        assert_eq!(jz.jump_target(0), Some(6));
    }

    #[test]
    fn display_is_human() {
        assert_eq!(Op::LoadConst(42).to_string(), "ldc 42");
        assert_eq!(Op::Jump(-3).to_string(), "jmp -3");
        assert_eq!(Op::MakeClosure(7,2).to_string(), "mkclo 7 2");
    }
}
