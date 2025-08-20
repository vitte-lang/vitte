//!
//! Assembleur/Désassembleur minimaliste mais *sérieux* pour la VM Vitte.
//! (… doc identique …)

use std::collections::{BTreeMap, HashMap};
use std::fmt;
use std::num::ParseIntError;
use std::str::FromStr;

// ==============================
// Erreurs
// ==============================

#[derive(Debug, Clone)]
pub struct AsmError {
    pub line: usize,
    pub col: usize,
    pub msg: String,
}

impl AsmError {
    fn new(line: usize, col: usize, msg: impl Into<String>) -> Self {
        Self { line, col, msg: msg.into() }
    }
}

impl fmt::Display for AsmError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "[{}:{}] {}", self.line, self.col, self.msg)
    }
}

impl std::error::Error for AsmError {}

// ==============================
// Lexing
// ==============================

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum TokKind {
    Ident,
    Int,
    Float,
    String,
    Comma,
    Colon,
    At,
    Eq,
    Dot,
    Plus,       // ← ajouté
    LBracket,   // [
    RBracket,   // ]
    Newline,
}

#[derive(Debug, Clone)]
struct Tok {
    kind: TokKind,
    text: String,
    line: usize,
    col: usize,
}

fn is_ident_start(c: char) -> bool {
    c.is_ascii_alphabetic() || c == '_' || c == '$'
}
fn is_ident_continue(c: char) -> bool {
    c.is_ascii_alphanumeric() || c == '_' || c == '-'
        || c == '.' // on autorise les mnémos/labels avec des points
}

fn lex(src: &str) -> Result<Vec<Tok>, AsmError> {
    let mut out = Vec::new();
    let mut line = 1usize;
    let mut col = 1usize;
    let mut it = src.chars().peekable();

    while let Some(&ch) = it.peek() {
        // commentaires
        if ch == ';' || (ch == '/' && it.clone().nth(1) == Some('/')) {
            while let Some(c) = it.next() {
                if c == '\n' { break; }
            }
            out.push(Tok { kind: TokKind::Newline, text: "\n".into(), line, col });
            line += 1; col = 1;
            continue;
        }
        if ch == '\n' {
            it.next();
            out.push(Tok { kind: TokKind::Newline, text: "\n".into(), line, col });
            line += 1; col = 1;
            continue;
        }
        if ch.is_whitespace() {
            it.next();
            col += 1;
            continue;
        }

        // ponctuation
        let single = match ch {
            ',' => Some(TokKind::Comma),
            ':' => Some(TokKind::Colon),
            '@' => Some(TokKind::At),
            '=' => Some(TokKind::Eq),
            '.' => Some(TokKind::Dot),
            '+' => Some(TokKind::Plus),      // ← ajouté
            '[' => Some(TokKind::LBracket),
            ']' => Some(TokKind::RBracket),
            _ => None,
        };
        if let Some(kind) = single {
            it.next();
            out.push(Tok { kind, text: ch.to_string(), line, col });
            col += 1;
            continue;
        }

        // string "..."
        if ch == '"' {
            let start_col = col;
            it.next();
            col += 1;
            let mut s = String::new();
            while let Some(c) = it.next() {
                col += 1;
                if c == '"' { break; }
                if c == '\\' {
                    if let Some(esc) = it.next() {
                        col += 1;
                        let r = match esc {
                            'n' => '\n',
                            't' => '\t',
                            'r' => '\r',
                            '\\' => '\\',
                            '"' => '"',
                            '0' => '\0',
                            _ => esc,
                        };
                        s.push(r);
                    } else {
                        return Err(AsmError::new(line, col, "escape incomplet"));
                    }
                } else {
                    s.push(c);
                }
            }
            out.push(Tok { kind: TokKind::String, text: s, line, col: start_col });
            continue;
        }

        // nombre (int/float, support 0x.., 0b..)
        if ch.is_ascii_digit() || (ch == '-' && it.clone().nth(1).map_or(false, |d| d.is_ascii_digit())) {
            let start_col = col;
            let mut s = String::new();
            s.push(ch);
            it.next(); col += 1;

            while let Some(&c) = it.peek() {
                if c.is_ascii_hexdigit() || c == 'x' || c == 'b' || c == '_' || c == '.' || c == '-' {
                    s.push(c);
                    it.next(); col += 1;
                } else {
                    break;
                }
            }
            if s.contains('.') && !s.starts_with("0x") && !s.starts_with("-0x") {
                out.push(Tok { kind: TokKind::Float, text: s, line, col: start_col });
            } else {
                out.push(Tok { kind: TokKind::Int, text: s, line, col: start_col });
            }
            continue;
        }

        // ident
        if is_ident_start(ch) {
            let start_col = col;
            let mut s = String::new();
            s.push(ch);
            it.next(); col += 1;
            while let Some(&c) = it.peek() {
                if is_ident_continue(c) {
                    s.push(c);
                    it.next(); col += 1;
                } else { break; }
            }
            out.push(Tok { kind: TokKind::Ident, text: s, line, col: start_col });
            continue;
        }

        return Err(AsmError::new(line, col, format!("caractère inattendu: {:?}", ch)));
    }

    Ok(out)
}

// ==============================
// AST & IR
// ==============================

#[derive(Debug, Clone)]
pub enum Operand {
    Reg(u8),         // r0..r255
    ImmI(i64),
    ImmF(f64),
    ConstRef(String), // const:name
    LabelRef(String), // @label
    Mem { base: u8, offset: i32 }, // [rX+imm] ou [rX, imm]
}

#[derive(Debug, Clone)]
pub struct Instr {
    pub line: usize,
    pub col: usize,
    pub mnemonic: String,
    pub operands: Vec<Operand>,
}

#[derive(Debug, Clone)]
pub enum Directive {
    Const { name: String, value: ConstValue, line: usize, col: usize },
    Align { to: u32, line: usize, col: usize },
    Org   { addr: u32, line: usize, col: usize },
    Entry { label: String, line: usize, col: usize },
    DataBytes { name: Option<String>, bytes: Vec<u8>, line: usize, col: usize },
    String { name: String, utf8: String, line: usize, col: usize },
}

#[derive(Debug, Clone)]
pub enum ConstValue {
    Int(i64),
    Float(f64),
    String(String),
}

#[derive(Debug, Default, Clone)]
pub struct Unit {
    pub directives: Vec<Directive>,
    pub items: Vec<Item>,
}

#[derive(Debug, Clone)]
pub enum Item {
    Label { name: String, line: usize, col: usize },
    Instr(Instr),
}

// ==============================
// Parsing
// ==============================

struct Parser<'a> {
    toks: &'a [Tok],
    i: usize,
}

impl<'a> Parser<'a> {
    fn new(toks: &'a [Tok]) -> Self { Self { toks, i: 0 } }

    fn peek(&self) -> Option<&'a Tok> { self.toks.get(self.i) }
    fn next(&mut self) -> Option<&'a Tok> { let t = self.toks.get(self.i); self.i += 1; t }
    fn eat_newlines(&mut self) {
        while matches!(self.peek(), Some(Tok{kind: TokKind::Newline, ..})) { self.i += 1; }
    }

    fn parse(mut self) -> Result<Unit, AsmError> {
        let mut unit = Unit::default();
        while self.peek().is_some() {
            self.eat_newlines();
            match self.peek() {
                Some(Tok{kind: TokKind::Dot, ..}) => {
                    let d = self.parse_directive()?;
                    unit.directives.push(d);
                }
                Some(Tok{kind: TokKind::Ident, text, line, col}) => {
                    if self.toks.get(self.i+1).map(|t| t.kind) == Some(TokKind::Colon) {
                        let name = text.clone();
                        let l = *line; let c = *col;
                        self.i += 2; // ident + ':'
                        unit.items.push(Item::Label { name, line: l, col: c });
                    } else {
                        let instr = self.parse_instr()?;
                        unit.items.push(Item::Instr(instr));
                    }
                }
                Some(Tok{kind: TokKind::Newline, ..}) => { self.i += 1; }
                Some(t) => {
                    return Err(AsmError::new(t.line, t.col, "attendu une directive, une étiquette ou une instruction"));
                }
                None => break,
            }
        }
        Ok(unit)
    }

    fn parse_directive(&mut self) -> Result<Directive, AsmError> {
        let dot = self.next().unwrap(); // '.'
        let name = self.expect_ident()?;
        match name.text.as_str() {
            "const" => {
                let id = self.expect_ident()?;
                self.expect(TokKind::Eq)?;
                let (cv, line, col) = self.parse_const_value()?;
                Ok(Directive::Const { name: id.text.clone(), value: cv, line, col })
            }
            "string" => {
                let id = self.expect_ident()?;
                self.expect(TokKind::Eq)?;
                let s = self.expect_string()?;
                Ok(Directive::String { name: id.text.clone(), utf8: s.text.clone(), line: id.line, col: id.col })
            }
            "data" => {
                // .data [optional_name] = [ bytes... ]
                let (name_opt, line, col) = if matches!(self.peek(), Some(Tok{kind: TokKind::Ident, ..})) &&
                    self.toks.get(self.i+1).map(|t| t.kind) == Some(TokKind::Eq) {
                    let id = self.expect_ident()?;
                    self.expect(TokKind::Eq)?;
                    (Some(id.text.clone()), id.line, id.col)
                } else {
                    (None, dot.line, dot.col)
                };
                let mut bytes = Vec::new();
                loop {
                    match self.peek() {
                        Some(Tok{kind: TokKind::Int, text, ..}) => {
                            let b = parse_int_byte(text).map_err(|_| AsmError::new(dot.line, dot.col, "octet invalide"))?;
                            bytes.push(b);
                            self.i += 1;
                        }
                        Some(Tok{kind: TokKind::Comma, ..}) => { self.i += 1; }
                        Some(Tok{kind: TokKind::Newline, ..}) | None => break,
                        Some(other) => {
                            return Err(AsmError::new(other.line, other.col, "attendu liste d’octets (0..255, 0x..)"));
                        }
                    }
                }
                Ok(Directive::DataBytes { name: name_opt, bytes, line, col })
            }
            "align" => {
                let n = self.expect_int()?;
                let val = parse_u32(&n.text).map_err(|e| AsmError::new(n.line, n.col, format!("align invalide: {e}")))?;
                Ok(Directive::Align { to: val, line: n.line, col: n.col })
            }
            "org" => {
                let n = self.expect_int()?;
                let val = parse_u32(&n.text).map_err(|e| AsmError::new(n.line, n.col, format!("org invalide: {e}")))?;
                Ok(Directive::Org { addr: val, line: n.line, col: n.col })
            }
            "entry" => {
                let id = self.expect_ident()?;
                Ok(Directive::Entry { label: id.text.clone(), line: id.line, col: id.col })
            }
            other => Err(AsmError::new(name.line, name.col, format!("directive inconnue: .{other}"))),
        }
    }

    fn parse_const_value(&mut self) -> Result<(ConstValue, usize, usize), AsmError> {
        match self.peek() {
            Some(Tok{kind: TokKind::Int, text, line, col}) => {
                self.i += 1;
                let v = parse_i64(text)
                    .map_err(|e| AsmError::new(*line, *col, format!("entier invalide: {e}")))?;
                Ok((ConstValue::Int(v), *line, *col))
            }
            Some(Tok{kind: TokKind::Float, text, line, col}) => {
                self.i += 1;
                let v: f64 = text.parse()
                    .map_err(|_| AsmError::new(*line, *col, "flottant invalide"))?;
                Ok((ConstValue::Float(v), *line, *col))
            }
            Some(Tok{kind: TokKind::String, text, line, col}) => {
                self.i += 1;
                Ok((ConstValue::String(text.clone()), *line, *col))
            }
            Some(t) => Err(AsmError::new(t.line, t.col, "valeur constante attendue")),
            None => Err(AsmError::new(0, 0, "fin inattendue")),
        }
    }

    fn parse_instr(&mut self) -> Result<Instr, AsmError> {
        let head = self.expect_ident()?;
        let mnemonic = head.text.to_uppercase();
        let line = head.line;
        let col = head.col;
        let mut ops = Vec::new();

        loop {
            match self.peek() {
                Some(Tok { kind: TokKind::Newline, .. }) | None => break,
                Some(Tok { kind: TokKind::Comma, .. }) => { self.i += 1; }
                _ => {
                    let op = self.parse_operand()?;
                    ops.push(op);
                }
            }
        }

        Ok(Instr { line, col, mnemonic, operands: ops })
    }

    fn parse_operand(&mut self) -> Result<Operand, AsmError> {
        match self.peek() {
            Some(Tok{kind: TokKind::Ident, text, line, col}) => {
                // reg? const:NAME ?
                if text.starts_with('r') || text.starts_with('R') {
                    let idx: u16 = text[1..].parse().map_err(|_| AsmError::new(*line, *col, "registre invalide"))?;
                    if idx > 255 { return Err(AsmError::new(*line, *col, "registre hors plage (0..255)")); }
                    self.i += 1;
                    Ok(Operand::Reg(idx as u8))
                } else if text == "const" && self.toks.get(self.i+1).map(|t| t.kind) == Some(TokKind::Colon) {
                    self.i += 2; // const :
                    let id = self.expect_ident()?;
                    Ok(Operand::ConstRef(id.text.clone()))
                } else {
                    self.i += 1;
                    Ok(Operand::ConstRef(text.clone()))
                }
            }
            Some(Tok{kind: TokKind::At, ..}) => {
                self.i += 1;
                let id = self.expect_ident()?;
                Ok(Operand::LabelRef(id.text.clone()))
            }
            Some(Tok{kind: TokKind::Int, text, line, col}) => {
                let val = parse_i64(text).map_err(|e| AsmError::new(*line, *col, format!("entier invalide: {e}")))?;
                self.i += 1;
                Ok(Operand::ImmI(val))
            }
            Some(Tok{kind: TokKind::Float, text, line, col}) => {
                let v: f64 = text.parse().map_err(|_| AsmError::new(*line, *col, "flottant invalide"))?;
                self.i += 1;
                Ok(Operand::ImmF(v))
            }
            Some(Tok{kind: TokKind::LBracket, ..}) => {
                // [rX], [rX, imm] ou [rX + imm]
                self.i += 1; // [
                let r = self.expect_ident()?;
                if !(r.text.starts_with('r') || r.text.starts_with('R')) {
                    return Err(AsmError::new(r.line, r.col, "attendu registre rX"));
                }
                let ridx: u16 = r.text[1..].parse().map_err(|_| AsmError::new(r.line, r.col, "registre invalide"))?;
                let mut offset: i32 = 0;

                // variantes : ']' | (',' imm ']') | ('+' imm ']')
                match self.peek().map(|t| t.kind) {
                    Some(TokKind::RBracket) => {
                        self.i += 1; // ]
                    }
                    Some(TokKind::Comma) => {
                        self.i += 1; // ,
                        let n = self.expect_int()?;
                        offset = parse_i64(&n.text).map_err(|e| AsmError::new(n.line, n.col, format!("offset invalide: {e}")))? as i32;
                        self.expect(TokKind::RBracket)?;
                    }
                    Some(TokKind::Plus) => {
                        self.i += 1; // +
                        let n = self.expect_int()?;
                        offset = parse_i64(&n.text).map_err(|e| AsmError::new(n.line, n.col, format!("offset invalide: {e}")))? as i32;
                        self.expect(TokKind::RBracket)?;
                    }
                    Some(other) => {
                        return Err(AsmError::new(r.line, r.col, format!("syntaxe mémoire : attendu ']', ',' ou '+', trouvé {:?}", other)));
                    }
                    None => return Err(AsmError::new(r.line, r.col, "fin inattendue dans []")),
                }

                Ok(Operand::Mem { base: ridx as u8, offset })
            }
            Some(t) => Err(AsmError::new(t.line, t.col, "opérande inattendu")),
            None => Err(AsmError::new(0, 0, "fin inattendue")),
        }
    }

    fn expect(&mut self, kind: TokKind) -> Result<&'a Tok, AsmError> {
        if let Some(t) = self.next() {
            if t.kind == kind { Ok(t) } else { Err(AsmError::new(t.line, t.col, format!("attendu {:?}", kind))) }
        } else {
            Err(AsmError::new(0, 0, "fin inattendue"))
        }
    }
    fn expect_ident(&mut self) -> Result<&'a Tok, AsmError> {
        if let Some(t) = self.next() {
            if t.kind == TokKind::Ident { Ok(t) } else { Err(AsmError::new(t.line, t.col, "identifiant attendu")) }
        } else { Err(AsmError::new(0,0,"fin inattendue")) }
    }
    fn expect_int(&mut self) -> Result<&'a Tok, AsmError> {
        if let Some(t) = self.next() {
            if t.kind == TokKind::Int { Ok(t) } else { Err(AsmError::new(t.line, t.col, "entier attendu")) }
        } else { Err(AsmError::new(0,0,"fin inattendue")) }
    }
    fn expect_string(&mut self) -> Result<&'a Tok, AsmError> {
        if let Some(t) = self.next() {
            if t.kind == TokKind::String { Ok(t) } else { Err(AsmError::new(t.line, t.col, "chaine attendue")) }
        } else { Err(AsmError::new(0,0,"fin inattendue")) }
    }
}

fn parse_int_byte(s: &str) -> Result<u8, ParseIntError> {
    let v = parse_u64(s)?;
    Ok((v & 0xFF) as u8)
}
fn parse_u32(s: &str) -> Result<u32, ParseIntError> {
    let v = parse_u64(s)?;
    Ok((v & 0xFFFF_FFFF) as u32)
}
fn parse_u64(s: &str) -> Result<u64, ParseIntError> {
    if let Some(hex) = s.strip_prefix("0x").or_else(|| s.strip_prefix("-0x")) {
        u64::from_str_radix(hex.replace('_', "").as_str(), 16)
    } else if let Some(bin) = s.strip_prefix("0b").or_else(|| s.strip_prefix("-0b")) {
        u64::from_str_radix(bin.replace('_', "").as_str(), 2)
    } else {
        u64::from_str(&s.replace('_', ""))
    }
}
fn parse_i64(s: &str) -> Result<i64, ParseIntError> {
    if s.starts_with("0x") || s.starts_with("-0x") {
        let neg = s.starts_with("-0x");
        let hex = s.trim_start_matches('-').trim_start_matches("0x").replace('_', "");
        let v = i64::from_str_radix(&hex, 16)?;
        Ok(if neg { -v } else { v })
    } else if s.starts_with("0b") || s.starts_with("-0b") {
        let neg = s.starts_with("-0b");
        let bin = s.trim_start_matches('-').trim_start_matches("0b").replace('_', "");
        let v = i64::from_str_radix(&bin, 2)?;
        Ok(if neg { -v } else { v })
    } else {
        i64::from_str(&s.replace('_', ""))
    }
}

// ==============================
// Table d’opcodes & encodage « raw »
// ==============================

#[derive(Debug, Clone, Copy)]
pub struct RawOp {
    pub opcode: u16,
    pub argc: u8,
    pub args: [u64; 3],
}

#[derive(Debug, Default, Clone)]
pub struct RawProgram {
    pub code: Vec<RawOp>,
    pub entry_pc: Option<usize>,
    pub const_pool: ConstPool,
    pub data_blobs: Vec<DataBlob>,
}

#[derive(Debug, Default, Clone)]
pub struct ConstPool {
    pub ints: BTreeMap<String, i64>,
    pub floats: BTreeMap<String, f64>,
    pub strings: BTreeMap<String, String>,
}

#[derive(Debug, Clone)]
pub struct DataBlob {
    pub name: Option<String>,
    pub bytes: Vec<u8>,
    pub addr: Option<u32>, // si .org utilisé
}

/// Décrit la “forme” d’une instruction pour validation/assemblage.
#[derive(Debug, Clone)]
pub struct OpSig {
    pub code: u16,
    pub name: &'static str,
    pub operands: &'static [OpArgKind],
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OpArgKind {
    Reg,
    ImmI,
    ImmF,
    ConstIx, // référence à .const/.string résolue en index (ici: hash stable)
    Label,   // résolue en PC
    Mem,     // [rX+imm]
}

#[derive(Debug, Default)]
pub struct OpcodeTable {
    by_name: HashMap<&'static str, OpSig>,
}

impl OpcodeTable {
    pub fn new_default() -> Self {
        let mut t = OpcodeTable { by_name: HashMap::new() };
        let defs = [
            OpSig { code: 0x00, name: "NOP",   operands: &[] },
            OpSig { code: 0x01, name: "MOV",   operands: &[OpArgKind::Reg, OpArgKind::Reg] },
            OpSig { code: 0x02, name: "LOADK", operands: &[OpArgKind::Reg, OpArgKind::ConstIx] },
            OpSig { code: 0x03, name: "LOADI", operands: &[OpArgKind::Reg, OpArgKind::ImmI] },
            OpSig { code: 0x04, name: "LOADF", operands: &[OpArgKind::Reg, OpArgKind::ImmF] },
            OpSig { code: 0x05, name: "ADD",   operands: &[OpArgKind::Reg, OpArgKind::Reg, OpArgKind::Reg] },
            OpSig { code: 0x06, name: "SUB",   operands: &[OpArgKind::Reg, OpArgKind::Reg, OpArgKind::Reg] },
            OpSig { code: 0x07, name: "MUL",   operands: &[OpArgKind::Reg, OpArgKind::Reg, OpArgKind::Reg] },
            OpSig { code: 0x08, name: "DIV",   operands: &[OpArgKind::Reg, OpArgKind::Reg, OpArgKind::Reg] },
            OpSig { code: 0x09, name: "JMP",   operands: &[OpArgKind::Label] },
            OpSig { code: 0x0A, name: "JZ",    operands: &[OpArgKind::Reg, OpArgKind::Label] },
            OpSig { code: 0x0B, name: "CALL",  operands: &[OpArgKind::Reg, OpArgKind::Reg, OpArgKind::ImmI] },
            OpSig { code: 0x0C, name: "RET",   operands: &[] },
            OpSig { code: 0x0D, name: "LOADM", operands: &[OpArgKind::Reg, OpArgKind::Mem] },
            OpSig { code: 0x0E, name: "STORM", operands: &[OpArgKind::Mem, OpArgKind::Reg] }, // nom gardé comme dans tes tests
        ];
        for d in defs { t.by_name.insert(d.name, d); }
        t
    }

    pub fn get(&self, name: &str) -> Option<&OpSig> {
        self.by_name.get(name)
    }

    pub fn insert(&mut self, sig: OpSig) {
        self.by_name.insert(sig.name, sig);
    }
}

// Résultat d’assemblage
#[derive(Debug, Clone)]
pub struct AssembleOutput {
    pub program: RawProgram,
    pub labels: BTreeMap<String, usize>,
}

/// Assemble entièrement un texte ASM en `RawProgram`.
pub fn assemble(src: &str) -> Result<AssembleOutput, AsmError> {
    let toks = lex(src)?;
    let unit = Parser::new(&toks).parse()?;
    assemble_unit(&unit, &OpcodeTable::new_default())
}

/// Assemble avec une table d’opcodes custom.
pub fn assemble_with_table(src: &str, table: &OpcodeTable) -> Result<AssembleOutput, AsmError> {
    let toks = lex(src)?;
    let unit = Parser::new(&toks).parse()?;
    assemble_unit(&unit, table)
}

fn assemble_unit(unit: &Unit, table: &OpcodeTable) -> Result<AssembleOutput, AsmError> {
    // 1) Constantes, strings, data, entry
    let mut pool = ConstPool::default();
    let mut data = Vec::<DataBlob>::new();
    let mut entry_label: Option<String> = None;
    let mut org_addr: Option<u32> = None;

    for d in &unit.directives {
        match d {
            Directive::Const { name, value, .. } => match value {
                ConstValue::Int(v) => { pool.ints.insert(name.clone(), *v); }
                ConstValue::Float(v) => { pool.floats.insert(name.clone(), *v); }
                ConstValue::String(s) => { pool.strings.insert(name.clone(), s.clone()); }
            },
            Directive::String { name, utf8, .. } => {
                pool.strings.insert(name.clone(), utf8.clone());
            }
            Directive::DataBytes { name, bytes, .. } => {
                data.push(DataBlob { name: name.clone(), bytes: bytes.clone(), addr: org_addr });
                org_addr = None;
            }
            Directive::Org { addr, .. } => { org_addr = Some(*addr); }
            Directive::Entry { label, .. } => { entry_label = Some(label.clone()); }
            Directive::Align { .. } => {
                // no-op ici (pas de segment code binaire)
            }
        }
    }

    // 2) Première passe — PC & labels
    let mut pc: usize = 0;
    let mut labels = BTreeMap::<String, usize>::new();
    for it in &unit.items {
        match it {
            Item::Label { name, .. } => {
                let _ = labels.insert(name.clone(), pc);
            }
            Item::Instr(_) => {
                pc += 1;
            }
        }
    }

    // 3) Seconde passe — encodage
    let mut code = Vec::<RawOp>::new();
    for it in &unit.items {
        if let Item::Instr(instr) = it {
            let sig = table.get(&instr.mnemonic)
                .ok_or_else(|| AsmError::new(instr.line, instr.col, format!("opcode inconnu: {}", instr.mnemonic)))?;

            if instr.operands.len() != sig.operands.len() {
                return Err(AsmError::new(
                    instr.line, instr.col,
                    format!("{} attend {} opérande(s), reçu {}", sig.name, sig.operands.len(), instr.operands.len())
                ));
            }

            let mut args = [0u64; 3];
            for (i, (op_ast, kind)) in instr.operands.iter().zip(sig.operands.iter()).enumerate() {
                args[i] = resolve_operand(op_ast, *kind, &pool, &labels)
                    .map_err(|m| AsmError::new(instr.line, instr.col, m))?;
            }

            code.push(RawOp { opcode: sig.code, argc: sig.operands.len() as u8, args });
        }
    }

    // 4) résout entry
    let entry_pc = match entry_label {
        None => None,
        Some(lbl) => labels.get(&lbl).copied(),
    };

    Ok(AssembleOutput {
        program: RawProgram { code, entry_pc, const_pool: pool, data_blobs: data },
        labels,
    })
}

// Paramètre _pool gardé pour un usage futur et pour éviter le warning.
fn resolve_operand(
    op: &Operand,
    kind: OpArgKind,
    _pool: &ConstPool,
    labels: &BTreeMap<String, usize>,
) -> Result<u64, String> {
    match (op, kind) {
        (Operand::Reg(r), OpArgKind::Reg) => Ok(*r as u64),
        (Operand::ImmI(i), OpArgKind::ImmI) => Ok(*i as u64),
        (Operand::ImmF(f), OpArgKind::ImmF) => Ok(f.to_bits()),
        (Operand::ConstRef(name), OpArgKind::ConstIx) => Ok(stable_hash(name)),
        (Operand::LabelRef(lbl), OpArgKind::Label) => {
            labels.get(lbl).copied().map(|pc| pc as u64).ok_or_else(|| format!("label inconnu: {lbl}"))
        }
        (Operand::Mem { base, offset }, OpArgKind::Mem) => {
            let b = (*base as u64) & 0xFF;
            let off = (*offset as i64 as u64) & 0xFFFF_FFFF;
            Ok((b << 56) | off)
        }
        (got, want) => Err(format!("mauvais type d’opérande: got={:?} want={:?}", got, want)),
    }
}

fn stable_hash(s: &str) -> u64 {
    const FNV_OFFSET: u64 = 0xcbf29ce484222325;
    const FNV_PRIME:  u64 = 0x100000001b3;
    let mut h = FNV_OFFSET;
    for b in s.as_bytes() {
        h ^= *b as u64;
        h = h.wrapping_mul(FNV_PRIME);
    }
    h
}

// ==============================
// Désassembleur
// ==============================

pub fn disassemble(prog: &RawProgram, table: &OpcodeTable) -> String {
    let mut rev: HashMap<u16, &OpSig> = HashMap::new();
    for sig in table.by_name.values() {
        rev.insert(sig.code, sig);
    }

    let mut lines = Vec::<String>::new();

    if let Some(pc) = prog.entry_pc {
        lines.push(format!(".entry L{}", pc));
    }

    for (k, v) in &prog.const_pool.ints {
        lines.push(format!(".const {} = {}", k, v));
    }
    for (k, v) in &prog.const_pool.floats {
        lines.push(format!(".const {} = {}", k, v));
    }
    for (k, v) in &prog.const_pool.strings {
        let esc = v.replace('\\', "\\\\").replace('"', "\\\"");
        lines.push(format!(".string {} = \"{}\"", k, esc));
    }

    for blob in &prog.data_blobs {
        if let Some(addr) = blob.addr {
            lines.push(format!(".org {}", addr));
        }
        if let Some(n) = &blob.name {
            lines.push(format!(".data {} = {}", n, bytes_as_list(&blob.bytes)));
        } else {
            lines.push(format!(".data {}", bytes_as_list(&blob.bytes)));
        }
    }

    for (pc, op) in prog.code.iter().enumerate() {
        lines.push(format!("L{}:", pc));
        if let Some(sig) = rev.get(&op.opcode).copied() {
            let mut parts = vec![sig.name.to_string()];
            for i in 0..(op.argc as usize) {
                let k = sig.operands[i];
                let a = op.args[i];
                parts.push(fmt_operand_rev(k, a));
            }
            lines.push(format!("    {}", parts.join(", ")));
        } else {
            let mut args = Vec::new();
            for i in 0..(op.argc as usize) {
                args.push(format!("0x{:016X}", op.args[i]));
            }
            lines.push(format!("    .raw 0x{:04X} {}", op.opcode, args.join(", ")));
        }
    }

    lines.join("\n")
}

fn bytes_as_list(bytes: &[u8]) -> String {
    let mut s = String::new();
    for (i, b) in bytes.iter().enumerate() {
        if i > 0 { s.push_str(", "); }
        s.push_str(&format!("{}", b));
    }
    s
}

fn fmt_operand_rev(kind: OpArgKind, a: u64) -> String {
    match kind {
        OpArgKind::Reg => format!("r{}", a as u8),
        OpArgKind::ImmI => format!("{}", a as i64),
        OpArgKind::ImmF => {
            let f = f64::from_bits(a);
            if f.fract() == 0.0 { format!("{:.1}", f) } else { format!("{}", f) }
        }
        OpArgKind::ConstIx => format!("const:0x{:016X}", a),
        OpArgKind::Label => format!("@L{}", a),
        OpArgKind::Mem => {
            let base = ((a >> 56) & 0xFF) as u8;
            let off = (a & 0xFFFF_FFFF) as u32 as i32;
            if off == 0 { format!("[r{}]", base) } else { format!("[r{}, {}]", base, off) }
        }
    }
}

// ==============================
// Pont vers votre `Op` maison
// ==============================

pub trait Encoder<Op> {
    type Error: std::error::Error + Send + Sync + 'static;
    fn encode(&self, raw: &RawOp) -> Result<Op, Self::Error>;
}

// ==============================
// Tests
// ==============================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roundtrip_basic() {
        let asm = r#"
            .entry main
            .const PI = 3.14
            .string msg = "Hello"
        main:
            NOP
            LOADI r0, 42
            LOADK r1, const:PI
            ADD r2, r0, r0
            JZ r2, @end
            CALL r3, r1, 0
        end:
            RET
        "#;

        let out = assemble(asm).expect("assemble");
        assert!(out.program.entry_pc.is_some());
        assert!(out.labels.contains_key("main"));
        assert!(out.labels.contains_key("end"));
        assert_eq!(out.program.code.len(), 7);

        let text = disassemble(&out.program, &OpcodeTable::new_default());
        assert!(text.contains(".entry"));
        assert!(text.contains(".const PI"));
        assert!(text.contains("L0:"));
        assert!(text.contains("NOP"));
        assert!(text.contains("RET"));
    }

    #[test]
    fn mem_operand_pack() {
        let asm = r#"
            main:
                LOADM r1, [r7, -16]
                LOADM r3, [r7 + 8]
                STORM [r7, 32], r2
                RET
        "#;
        let out = assemble(asm).expect("assemble");
        assert_eq!(out.program.code.len(), 4);
        let d = disassemble(&out.program, &OpcodeTable::new_default());
        assert!(d.contains("[r7, -16]"));
        assert!(d.contains("[r7, 8]"));
        assert!(d.contains("[r7, 32]"));
    }
}
