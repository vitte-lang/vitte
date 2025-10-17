use std::num::NonZeroU32;

use smol_str::SmolStr;

macro_rules! define_id {
    ($name:ident) => {
        #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
        pub struct $name(NonZeroU32);

        impl $name {
            pub fn from_index(index: usize) -> Self {
                let raw = (index + 1) as u32;
                Self(NonZeroU32::new(raw).expect("Id overflow"))
            }

            pub fn index(self) -> usize {
                self.0.get() as usize - 1
            }
        }
    };
}

define_id!(ModuleId);
define_id!(ItemId);
define_id!(ExprId);
define_id!(PatternId);
define_id!(TypeId);
define_id!(BlockId);
define_id!(SpanId);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Span {
    pub start: u32,
    pub end: u32,
}

impl Span {
    pub fn new(start: u32, end: u32) -> Self {
        Self { start, end }
    }

    pub fn dummy() -> Self {
        Self { start: 0, end: 0 }
    }
}

#[derive(Clone, Debug)]
pub struct Ident {
    pub name: SmolStr,
    pub span: Span,
}

impl Ident {
    pub fn new(name: impl Into<SmolStr>, span: Span) -> Self {
        Self { name: name.into(), span }
    }

    pub fn dummy() -> Self {
        Self::new("", Span::dummy())
    }
}

pub struct ModuleArena<T> {
    storage: Vec<T>,
}

pub struct ItemArena<T> {
    storage: Vec<T>,
}

pub struct ExprArena<T> {
    storage: Vec<T>,
}

pub struct PatternArena<T> {
    storage: Vec<T>,
}

pub struct TypeArena<T> {
    storage: Vec<T>,
}

pub struct BlockArena<T> {
    storage: Vec<T>,
}

pub struct SpanArena {
    spans: Vec<Span>,
}

pub struct AstStorage {
    pub modules: ModuleArena<ModuleData>,
    pub items: ItemArena<ItemData>,
    pub exprs: ExprArena<ExprData>,
    pub patterns: PatternArena<PatternData>,
    pub types: TypeArena<TypeData>,
    pub blocks: BlockArena<BlockData>,
    pub spans: SpanArena,
}

impl AstStorage {
    pub fn new() -> Self {
        Self {
            modules: ModuleArena::new(),
            items: ItemArena::new(),
            exprs: ExprArena::new(),
            patterns: PatternArena::new(),
            types: TypeArena::new(),
            blocks: BlockArena::new(),
            spans: SpanArena::new(),
        }
    }
}

pub struct Ast {
    pub storage: AstStorage,
    pub root: ModuleId,
}

impl Ast {
    pub fn new() -> Self {
        let mut storage = AstStorage::new();
        let root_span = storage.spans.alloc(Span::dummy());
        let root_module =
            storage
                .modules
                .alloc(ModuleData { name: None, items: Vec::new(), span: root_span });
        Self { storage, root: root_module }
    }

    pub fn root(&self) -> ModuleId {
        self.root
    }
}

impl<T> ModuleArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> ModuleId {
        let idx = self.storage.len();
        self.storage.push(value);
        ModuleId::from_index(idx)
    }

    pub fn get(&self, id: ModuleId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: ModuleId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl<T> ItemArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> ItemId {
        let idx = self.storage.len();
        self.storage.push(value);
        ItemId::from_index(idx)
    }

    pub fn get(&self, id: ItemId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: ItemId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl<T> ExprArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> ExprId {
        let idx = self.storage.len();
        self.storage.push(value);
        ExprId::from_index(idx)
    }

    pub fn get(&self, id: ExprId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: ExprId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl<T> PatternArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> PatternId {
        let idx = self.storage.len();
        self.storage.push(value);
        PatternId::from_index(idx)
    }

    pub fn get(&self, id: PatternId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: PatternId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl<T> TypeArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> TypeId {
        let idx = self.storage.len();
        self.storage.push(value);
        TypeId::from_index(idx)
    }

    pub fn get(&self, id: TypeId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: TypeId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl<T> BlockArena<T> {
    pub fn new() -> Self {
        Self { storage: Vec::new() }
    }

    pub fn alloc(&mut self, value: T) -> BlockId {
        let idx = self.storage.len();
        self.storage.push(value);
        BlockId::from_index(idx)
    }

    pub fn get(&self, id: BlockId) -> Option<&T> {
        self.storage.get(id.index())
    }

    pub fn get_mut(&mut self, id: BlockId) -> Option<&mut T> {
        self.storage.get_mut(id.index())
    }
}

impl SpanArena {
    pub fn new() -> Self {
        Self { spans: vec![Span::dummy()] }
    }

    pub fn alloc(&mut self, span: Span) -> SpanId {
        let idx = self.spans.len();
        self.spans.push(span);
        SpanId::from_index(idx)
    }

    pub fn get(&self, id: SpanId) -> Option<Span> {
        self.spans.get(id.index()).copied()
    }
}

// ───────────────────────────── AST nodes ─────────────────────────────

#[derive(Clone, Debug)]
pub struct ModuleData {
    pub name: Option<Ident>,
    pub items: Vec<ItemId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum ItemData {
    Use(UseItem),
    Struct(StructItem),
    Enum(EnumItem),
    Union(UnionItem),
    TypeAlias(TypeAliasItem),
    Fn(FnItem),
    Const(ConstItem),
    Static(StaticItem),
    Module(ModuleItem),
}

#[derive(Clone, Debug)]
pub struct ModuleItem {
    pub vis: Visibility,
    pub name: Ident,
    pub items: Vec<ItemId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct UseItem {
    pub vis: Visibility,
    pub tree: UseTree,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum UseTree {
    Path(Path),
    Glob(Path),
    Rename { path: Path, alias: Ident, span: SpanId },
    Nest { prefix: Option<Path>, items: Vec<UseTree>, span: SpanId },
}

#[derive(Clone, Debug)]
pub struct StructItem {
    pub vis: Visibility,
    pub name: Ident,
    pub generics: GenericParams,
    pub fields: StructFields,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum StructFields {
    Named(Vec<Field>),
    Unit(SpanId),
}

#[derive(Clone, Debug)]
pub struct Field {
    pub vis: Visibility,
    pub name: Ident,
    pub ty: TypeId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct EnumItem {
    pub vis: Visibility,
    pub name: Ident,
    pub generics: GenericParams,
    pub variants: Vec<EnumVariant>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct EnumVariant {
    pub name: Ident,
    pub data: VariantData,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum VariantData {
    Unit,
    Tuple(Vec<TypeId>),
    Struct(Vec<Field>),
}

#[derive(Clone, Debug)]
pub struct UnionItem {
    pub vis: Visibility,
    pub name: Ident,
    pub generics: GenericParams,
    pub fields: Vec<Field>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct TypeAliasItem {
    pub vis: Visibility,
    pub name: Ident,
    pub generics: GenericParams,
    pub ty: TypeId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct ConstItem {
    pub vis: Visibility,
    pub name: Ident,
    pub ty: TypeId,
    pub value: ExprId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct StaticItem {
    pub vis: Visibility,
    pub name: Ident,
    pub ty: TypeId,
    pub value: ExprId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct FnItem {
    pub vis: Visibility,
    pub name: Ident,
    pub generics: GenericParams,
    pub params: Vec<Param>,
    pub ret: Option<TypeId>,
    pub where_clauses: Vec<WherePredicate>,
    pub body: BlockId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct Param {
    pub pattern: PatternId,
    pub ty: TypeId,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct WherePredicate {
    pub target: TypeId,
    pub bounds: Vec<TypeId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct GenericParams {
    pub params: Vec<GenericParam>,
    pub span: Option<SpanId>,
}

#[derive(Clone, Debug)]
pub struct GenericParam {
    pub name: Ident,
    pub bounds: Vec<TypeId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct BlockData {
    pub stmts: Vec<Stmt>,
    pub tail: Option<ExprId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum Stmt {
    Let { pattern: PatternId, ty: Option<TypeId>, value: Option<ExprId>, span: SpanId },
    Item(ItemId),
    Expr { expr: ExprId, has_semicolon: bool, span: SpanId },
    Empty { span: SpanId },
}

#[derive(Clone, Debug)]
pub enum ExprData {
    Literal(Literal),
    Path(Path),
    Tuple { elements: Vec<ExprId>, span: SpanId },
    Array { elements: Vec<ExprId>, span: SpanId },
    ArrayRepeat { value: ExprId, count: ExprId, span: SpanId },
    Struct { path: Path, fields: Vec<StructExprField>, rest: Option<ExprId>, span: SpanId },
    Block(BlockId),
    If { cond: ExprId, then_branch: BlockId, else_branch: Option<ElseBranch>, span: SpanId },
    Match { scrutinee: ExprId, arms: Vec<MatchArm>, span: SpanId },
    Loop { body: BlockId, span: SpanId },
    While { cond: WhileCond, body: BlockId, span: SpanId },
    For { pattern: PatternId, iterable: ExprId, body: BlockId, span: SpanId },
    Break { value: Option<ExprId>, span: SpanId },
    Continue { span: SpanId },
    Return { value: Option<ExprId>, span: SpanId },
    Closure(ClosureExpr),
    Call { callee: ExprId, args: Vec<ExprId>, span: SpanId },
    Field { base: ExprId, member: Ident, span: SpanId },
    Index { base: ExprId, index: ExprId, span: SpanId },
    Try { expr: ExprId, span: SpanId },
    Unary { op: UnaryOp, expr: ExprId, span: SpanId },
    Binary { op: BinaryOp, left: ExprId, right: ExprId, span: SpanId },
    Assign { op: AssignOp, left: ExprId, right: ExprId, span: SpanId },
}

#[derive(Clone, Debug)]
pub struct StructExprField {
    pub name: Ident,
    pub value: Option<ExprId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct MatchArm {
    pub patterns: Vec<PatternId>,
    pub guard: Option<ExprId>,
    pub body: MatchArmBody,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum MatchArmBody {
    Expr(ExprId),
    Block(BlockId),
}

#[derive(Clone, Debug)]
pub enum ElseBranch {
    Block(BlockId),
    If(ExprId),
}

#[derive(Clone, Debug)]
pub enum WhileCond {
    Expr(ExprId),
    Let { pattern: PatternId, expr: ExprId, span: SpanId },
}

#[derive(Clone, Debug)]
pub struct ClosureExpr {
    pub is_async: bool,
    pub is_move: bool,
    pub params: Vec<ClosureParam>,
    pub ret: Option<TypeId>,
    pub body: ClosureBody,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub struct ClosureParam {
    pub pattern: PatternId,
    pub ty: Option<TypeId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum ClosureBody {
    Expr(ExprId),
    Block(BlockId),
}

#[derive(Clone, Debug)]
pub enum PatternData {
    Wildcard(SpanId),
    Literal(Literal),
    Ident { mutable: bool, name: Ident, span: SpanId },
    Tuple { elements: Vec<PatternId>, span: SpanId },
    Struct { path: Path, fields: Vec<StructPatternField>, rest: Option<SpanId>, span: SpanId },
    TupleStruct { path: Path, fields: Vec<PatternId>, span: SpanId },
    Ref { mutable: bool, pattern: PatternId, span: SpanId },
    Path(Path),
}

#[derive(Clone, Debug)]
pub struct StructPatternField {
    pub name: Ident,
    pub pattern: Option<PatternId>,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum TypeData {
    Path(Path),
    Tuple { elements: Vec<TypeId>, span: SpanId },
    Array { ty: TypeId, len: ExprId, span: SpanId },
    Slice { ty: TypeId, span: SpanId },
    Pointer { mutable: bool, ty: TypeId, span: SpanId },
    Reference { mutable: bool, ty: TypeId, span: SpanId },
    Fn { params: Vec<TypeId>, ret: Option<TypeId>, span: SpanId },
    Primitive(PrimitiveType, SpanId),
}

#[derive(Clone, Debug)]
pub struct Path {
    pub segments: Vec<PathSegment>,
    pub leading_colon: bool,
    pub span: SpanId,
}

impl Path {
    pub fn dummy() -> Self {
        Self { segments: Vec::new(), leading_colon: false, span: SpanId::from_index(0) }
    }
}

#[derive(Clone, Debug)]
pub struct PathSegment {
    pub ident: Ident,
    pub generic_args: Option<Vec<TypeId>>,
}

#[derive(Clone, Debug)]
pub struct Literal {
    pub kind: LiteralKind,
    pub span: SpanId,
}

#[derive(Clone, Debug)]
pub enum LiteralKind {
    Bool(bool),
    Nil,
    Int { text: SmolStr },
    Float { text: SmolStr },
    Char { value: char, raw: SmolStr },
    String { value: SmolStr, raw: SmolStr },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Visibility {
    Private,
    Public,
}

impl Default for Visibility {
    fn default() -> Self {
        Visibility::Private
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnaryOp {
    Not,
    Neg,
    BitNot,
    Deref,
    AddrOfMut,
    AddrOf,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
    Shl,
    Shr,
    BitAnd,
    BitOr,
    BitXor,
    LogicalAnd,
    LogicalOr,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum AssignOp {
    Assign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign,
    RemAssign,
    BitAndAssign,
    BitOrAssign,
    BitXorAssign,
    ShlAssign,
    ShrAssign,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PrimitiveType {
    I8,
    I16,
    I32,
    I64,
    Isize,
    U8,
    U16,
    U32,
    U64,
    Usize,
    F32,
    F64,
    Bool,
    Char,
    Str,
    Void,
}
