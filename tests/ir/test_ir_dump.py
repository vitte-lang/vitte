from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum, auto
from typing import List, Optional
import unittest


class TypeKind(Enum):
    UNIT = auto()
    BOOL = auto()
    I64 = auto()
    STR = auto()
    TUPLE = auto()
    UNKNOWN = auto()


@dataclass
class Type:
    kind: TypeKind
    name: str = ""
    items: List["Type"] = field(default_factory=list)

    @staticmethod
    def unit() -> "Type":
        return Type(TypeKind.UNIT)

    @staticmethod
    def bool() -> "Type":
        return Type(TypeKind.BOOL)

    @staticmethod
    def i64() -> "Type":
        return Type(TypeKind.I64)

    @staticmethod
    def str() -> "Type":
        return Type(TypeKind.STR)

    @staticmethod
    def tuple(items: List["Type"]) -> "Type":
        return Type(TypeKind.TUPLE, name=tuple_type_name(items), items=items)

    @staticmethod
    def unknown(name: str = "") -> "Type":
        return Type(TypeKind.UNKNOWN, name=name)


def tuple_type_name(items: List[Type]) -> str:
    return "(" + ", ".join(type_name(t) for t in items) + ")"


def type_name(t: Type) -> str:
    if t.kind is TypeKind.UNIT:
        return "unit"
    if t.kind is TypeKind.BOOL:
        return "bool"
    if t.kind is TypeKind.I64:
        return "i64"
    if t.kind is TypeKind.STR:
        return "str"
    if t.kind is TypeKind.TUPLE:
        return t.name or tuple_type_name(t.items)
    if t.kind is TypeKind.UNKNOWN:
        return t.name or "unknown"
    return "unknown"


@dataclass
class ValueId:
    raw: int


@dataclass
class Param:
    name: str
    ty: Type
    value: ValueId


class BinOp(Enum):
    ADD = auto()
    SUB = auto()
    MUL = auto()
    DIV = auto()
    EQ = auto()
    NE = auto()
    LT = auto()
    LE = auto()
    GT = auto()
    GE = auto()


class UnOp(Enum):
    NEG = auto()
    NOT = auto()


class InstrTag(Enum):
    CONST_INT = auto()
    CONST_BOOL = auto()
    CONST_STRING = auto()
    PARAM = auto()
    PHI = auto()
    BINOP = auto()
    UNOP = auto()
    CALL = auto()
    MAKE_TUPLE = auto()


@dataclass
class Instr:
    tag: InstrTag
    result: ValueId
    ty: Type
    value: Optional[str] = None
    bool_value: Optional[bool] = None
    incomings: Optional[List[tuple["BlockId", ValueId]]] = None
    binop: Optional[BinOp] = None
    lhs: Optional[ValueId] = None
    rhs: Optional[ValueId] = None
    unop: Optional[UnOp] = None
    operand: Optional[ValueId] = None
    callee: Optional[str] = None
    args: Optional[List[ValueId]] = None
    items: Optional[List[ValueId]] = None


@dataclass
class BlockId:
    raw: int


@dataclass
class Terminator:
    kind: str
    value: Optional[ValueId] = None
    target: Optional[BlockId] = None
    then_tgt: Optional[BlockId] = None
    else_tgt: Optional[BlockId] = None


@dataclass
class Block:
    id: BlockId
    name: str
    instrs: List[Instr]
    terminator: Optional[Terminator]


@dataclass
class Function:
    name: str
    params: List[Param]
    ret_type: Type
    blocks: List[Block]
    entry: BlockId


@dataclass
class Module:
    name: str
    functions: List[Function]


@dataclass
class Program:
    modules: List[Module]


def join_with(parts: List[str], sep: str) -> str:
    return sep.join(parts)


def fmt_type(t: Type) -> str:
    return type_name(t)


def fmt_value(v: ValueId) -> str:
    return f"%{v.raw}"


def fmt_block_id(b: BlockId) -> str:
    return f"bb{b.raw}"


def fmt_binop(op: BinOp) -> str:
    return {
        BinOp.ADD: "add",
        BinOp.SUB: "sub",
        BinOp.MUL: "mul",
        BinOp.DIV: "div",
        BinOp.EQ: "eq",
        BinOp.NE: "ne",
        BinOp.LT: "lt",
        BinOp.LE: "le",
        BinOp.GT: "gt",
        BinOp.GE: "ge",
    }[op]


def fmt_unop(op: UnOp) -> str:
    return {UnOp.NEG: "neg", UnOp.NOT: "not"}[op]


def fmt_incomings(incs: List[tuple[BlockId, ValueId]]) -> str:
    return join_with([f"{fmt_block_id(b)}:{fmt_value(v)}" for (b, v) in incs], ", ")


def fmt_instr(inst: Instr) -> str:
    if inst.tag is InstrTag.CONST_INT:
        return f"{fmt_value(inst.result)} = const {inst.value} : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.CONST_BOOL:
        lit = "true" if inst.bool_value else "false"
        return f"{fmt_value(inst.result)} = const {lit} : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.CONST_STRING:
        return f'{fmt_value(inst.result)} = const "{inst.value}" : {fmt_type(inst.ty)}'
    if inst.tag is InstrTag.PARAM:
        return f"{fmt_value(inst.result)} = param {inst.value} : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.PHI:
        return f"{fmt_value(inst.result)} = phi [{fmt_incomings(inst.incomings or [])}] : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.BINOP:
        return (
            f"{fmt_value(inst.result)} = {fmt_binop(inst.binop)} "
            f"{fmt_value(inst.lhs)}"
            f", {fmt_value(inst.rhs)} : {fmt_type(inst.ty)}"
        )
    if inst.tag is InstrTag.UNOP:
        return f"{fmt_value(inst.result)} = {fmt_unop(inst.unop)} {fmt_value(inst.operand)} : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.CALL:
        arg_strings = join_with([fmt_value(v) for v in inst.args or []], ", ")
        return f"{fmt_value(inst.result)} = call {inst.callee}({arg_strings}) : {fmt_type(inst.ty)}"
    if inst.tag is InstrTag.MAKE_TUPLE:
        parts = join_with([fmt_value(v) for v in inst.items or []], ", ")
        return f"{fmt_value(inst.result)} = tuple ({parts}) : {fmt_type(inst.ty)}"
    raise AssertionError(f"Unhandled instruction tag: {inst.tag}")


def fmt_terminator(t: Terminator) -> str:
    if t.kind == "return":
        if t.value is None:
            return "ret"
        return f"ret {fmt_value(t.value)}"
    if t.kind == "jump":
        return f"jmp {fmt_block_id(t.target)}"
    if t.kind == "cond_jump":
        return f"br {fmt_value(t.value)}, {fmt_block_id(t.then_tgt)}, {fmt_block_id(t.else_tgt)}"
    raise AssertionError(f"Unhandled terminator: {t.kind}")


def fmt_params(params: List[Param]) -> str:
    return join_with([f"{fmt_value(p.value)}: {fmt_type(p.ty)}" for p in params], ", ")


def fmt_block(block: Block) -> str:
    lines = [f"  block {fmt_block_id(block.id)} ({block.name}):"]
    for inst in block.instrs:
        lines.append(f"    {fmt_instr(inst)}")
    if block.terminator:
        lines.append(f"    {fmt_terminator(block.terminator)}")
    else:
        lines.append("    <no terminator>")
    return join_with(lines, "\n")


def fmt_function(fn: Function) -> str:
    lines = [f"fn {fn.name}({fmt_params(fn.params)}) -> {fmt_type(fn.ret_type)}"]
    lines.extend(fmt_block(b) for b in fn.blocks)
    return join_with(lines, "\n")


def fmt_module(mod: Module) -> str:
    lines = [f"module {mod.name}"]
    lines.extend(fmt_function(f) for f in mod.functions)
    return join_with(lines, "\n\n")


def format_program(p: Program) -> str:
    return join_with([fmt_module(m) for m in p.modules], "\n\n") + "\n"


def build_sample_program() -> Program:
    params = [
        Param(name="a", ty=Type.i64(), value=ValueId(0)),
        Param(name="flag", ty=Type.bool(), value=ValueId(1)),
    ]

    entry_block = Block(
        id=BlockId(0),
        name="entry",
        instrs=[
            Instr(
                tag=InstrTag.CONST_STRING,
                result=ValueId(2),
                ty=Type.str(),
                value="hello",
            ),
            Instr(
                tag=InstrTag.UNOP,
                result=ValueId(3),
                ty=Type.i64(),
                unop=UnOp.NEG,
                operand=ValueId(0),
            ),
            Instr(
                tag=InstrTag.UNOP,
                result=ValueId(4),
                ty=Type.bool(),
                unop=UnOp.NOT,
                operand=ValueId(1),
            ),
            Instr(
                tag=InstrTag.CALL,
                result=ValueId(5),
                ty=Type.unknown(),
                callee="do_call",
                args=[ValueId(3), ValueId(4)],
            ),
            Instr(
                tag=InstrTag.MAKE_TUPLE,
                result=ValueId(6),
                ty=Type.tuple([Type.i64(), Type.str()]),
                items=[ValueId(3), ValueId(2)],
            ),
        ],
        terminator=Terminator(kind="return", value=ValueId(6)),
    )

    fn = Function(
        name="main",
        params=params,
        ret_type=Type.tuple([Type.i64(), Type.str()]),
        blocks=[entry_block],
        entry=BlockId(0),
    )

    module = Module(name="sample.ir", functions=[fn])
    return Program(modules=[module])


class IrDumpFormatTest(unittest.TestCase):
    def test_format_program_covers_core_instrs(self) -> None:
        program = build_sample_program()
        formatted = format_program(program)
        expected = (
            "module sample.ir\n"
            "\n"
            "fn main(%0: i64, %1: bool) -> (i64, str)\n"
            "  block bb0 (entry):\n"
            '    %2 = const "hello" : str\n'
            "    %3 = neg %0 : i64\n"
            "    %4 = not %1 : bool\n"
            "    %5 = call do_call(%3, %4) : unknown\n"
            "    %6 = tuple (%3, %2) : (i64, str)\n"
            "    ret %6\n"
        )
        self.assertEqual(formatted, expected)


if __name__ == "__main__":
    unittest.main()
