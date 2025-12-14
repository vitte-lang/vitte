from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum, Enum, auto
from typing import List
import unittest


class ConstTag(IntEnum):
    NIL = 0
    BOOL = 1
    I64 = 2
    F64 = 3
    STRING = 4


class Opcode(IntEnum):
    OP_CONST = 0
    OP_ADD = 1
    OP_SUB = 2
    OP_CMP_EQ = 3
    OP_JMP = 4
    OP_JMP_IF = 5
    OP_RET = 6
    OP_MUL = 7
    OP_DIV = 8
    OP_MOD = 9
    OP_NEG = 10
    OP_CMP_NE = 11
    OP_CMP_LT = 12
    OP_CMP_LE = 13
    OP_CMP_GT = 14
    OP_CMP_GE = 15
    OP_LOAD_LOCAL = 16
    OP_STORE_LOCAL = 17
    OP_LOAD_FIELD = 18
    OP_STORE_FIELD = 19
    OP_ALLOC_HEAP = 20
    OP_CALL = 21
    OP_CALL_INDIRECT = 22
    OP_STD_PRINT = 23
    OP_STD_PRINTLN = 24
    OP_STD_MAKE_STRING = 25
    OP_STD_CONCAT_STRING = 26
    OP_STD_ARRAY_PUSH = 27
    OP_STD_ARRAY_GET = 28


class VmValueTag(Enum):
    NIL = auto()
    BOOL = auto()
    I64 = auto()
    F64 = auto()
    STRING = auto()
    ARRAY = auto()
    STRUCT = auto()


class HeapTag(Enum):
    STRING = auto()
    ARRAY = auto()
    STRUCT = auto()


@dataclass
class VmValue:
    tag: VmValueTag
    value: object


@dataclass
class Const:
    tag: ConstTag
    value: object


@dataclass
class FunctionEntry:
    name_const: int
    code_offset: int
    code_size: int
    param_count: int
    local_count: int
    max_stack: int = 8
    flags: int = 0


@dataclass
class VmFrame:
    func_index: int
    pc: int
    locals: List[VmValue]
    stack_base: int


@dataclass
class HeapObject:
    tag: HeapTag
    payload: object


@dataclass
class VmState:
    const_pool: List[Const]
    functions: List[FunctionEntry]
    code: List[int]
    frames: List[VmFrame] = field(default_factory=list)
    stack: List[VmValue] = field(default_factory=list)
    heap: List[HeapObject] = field(default_factory=list)
    printed: List[str] = field(default_factory=list)


@dataclass
class RtString:
    bytes: List[int]


@dataclass
class RtArray:
    len: int
    cap: int
    data_ptr: int


class StdHooks:
    def __init__(self, state: VmState):
        self.state = state

    def make_string(self, data: List[int]) -> RtString:
        return RtString(list(data))

    def concat_string(self, lhs: RtString, rhs: RtString) -> RtString:
        return RtString(lhs.bytes + rhs.bytes)

    def array_push(self, arr: RtArray, value: VmValue) -> RtArray:
        return arr

    def array_get(self, arr: RtArray, index: int) -> VmValue:
        return VmValue(VmValueTag.I64, index)

    def print(self, rt: RtString) -> None:
        self.state.printed.append(bytes(rt.bytes).decode("utf-8"))

    def println(self, rt: RtString) -> None:
        self.state.printed.append(bytes(rt.bytes).decode("utf-8") + "\n")

    def read_line(self) -> RtString:
        return RtString([])


def encode_inst(opcode: Opcode, *operands: int) -> List[int]:
    out = [int(opcode), len(operands)]
    for op in operands:
        out.extend([
            op & 0xFF,
            (op >> 8) & 0xFF,
            (op >> 16) & 0xFF,
            (op >> 24) & 0xFF,
        ])
    return out


def validate_code_bytes(code: List[int]) -> str:
    pc = 0
    while pc < len(code):
        if pc + 2 > len(code):
            return f"truncated instruction header at byte {pc}"
        opcode = code[pc]
        operand_count = code[pc + 1]
        expected = -1
        if opcode == Opcode.OP_CONST:
            expected = 1
        if opcode in (Opcode.OP_ADD, Opcode.OP_SUB, Opcode.OP_CMP_EQ, Opcode.OP_RET, Opcode.OP_MUL,
                      Opcode.OP_DIV, Opcode.OP_MOD, Opcode.OP_NEG, Opcode.OP_CMP_NE,
                      Opcode.OP_CMP_LT, Opcode.OP_CMP_LE, Opcode.OP_CMP_GT, Opcode.OP_CMP_GE):
            expected = 0
        if opcode in (Opcode.OP_JMP, Opcode.OP_JMP_IF, Opcode.OP_LOAD_LOCAL, Opcode.OP_STORE_LOCAL,
                      Opcode.OP_LOAD_FIELD, Opcode.OP_STORE_FIELD, Opcode.OP_ALLOC_HEAP, Opcode.OP_CALL):
            expected = 1
        if opcode == Opcode.OP_CALL_INDIRECT:
            expected = 0
        if opcode in (Opcode.OP_STD_PRINT, Opcode.OP_STD_PRINTLN, Opcode.OP_STD_CONCAT_STRING,
                      Opcode.OP_STD_ARRAY_PUSH, Opcode.OP_STD_ARRAY_GET):
            expected = 0
        if opcode == Opcode.OP_STD_MAKE_STRING:
            expected = 1
        if expected == -1:
            return f"unknown opcode byte {opcode} at byte {pc}"
        if operand_count != expected:
            return f"opcode {opcode} expects {expected} operands, found {operand_count}"
        inst_size = 2 + operand_count * 4
        if pc + inst_size > len(code):
            return f"instruction at byte {pc} truncated (needs {inst_size} bytes)"
        pc += inst_size
    if pc != len(code):
        return "code stream has trailing bytes"
    return ""


def decode_at_pc(code: List[int], pc: int) -> tuple[Opcode, List[int], int]:
    opcode = Opcode(code[pc])
    operand_count = code[pc + 1]
    operands = []
    for i in range(operand_count):
        base = pc + 2 + i * 4
        val = code[base] | (code[base + 1] << 8) | (code[base + 2] << 16) | (code[base + 3] << 24)
        operands.append(val)
    size = 2 + operand_count * 4
    return opcode, operands, size


def heap_alloc(heap: List[HeapObject], tag: HeapTag) -> int:
    heap.append(HeapObject(tag, []))
    return len(heap) - 1


def vm_value_from_const(state: VmState, const_index: int, hooks: StdHooks) -> VmValue:
    constant = state.const_pool[const_index]
    if constant.tag is ConstTag.NIL:
        return VmValue(VmValueTag.NIL, None)
    if constant.tag is ConstTag.BOOL:
        return VmValue(VmValueTag.BOOL, bool(constant.value))
    if constant.tag is ConstTag.I64:
        return VmValue(VmValueTag.I64, int(constant.value))
    if constant.tag is ConstTag.F64:
        return VmValue(VmValueTag.F64, float(constant.value))
    if constant.tag is ConstTag.STRING:
        rt = hooks.make_string(list(constant.value.encode("utf-8")))
        ref = heap_alloc(state.heap, HeapTag.STRING)
        state.heap[ref].payload = list(rt.bytes)
        return VmValue(VmValueTag.STRING, ref)
    return VmValue(VmValueTag.NIL, None)


def vm_render_value_bytes(state: VmState, value: VmValue) -> List[int]:
    if value.tag is VmValueTag.STRING:
        ref = value.value
        if 0 <= ref < len(state.heap) and state.heap[ref].tag is HeapTag.STRING:
            return list(state.heap[ref].payload)
    if value.tag is VmValueTag.I64:
        return list(str(value.value).encode("utf-8"))
    if value.tag is VmValueTag.BOOL:
        return list(str(value.value).encode("utf-8"))
    return list("<nil>".encode("utf-8"))


def vm_equals(state: VmState, lhs: VmValue, rhs: VmValue) -> bool:
    if lhs.tag is not rhs.tag:
        return False
    if lhs.tag in (VmValueTag.BOOL, VmValueTag.I64, VmValueTag.F64):
        return lhs.value == rhs.value
    if lhs.tag is VmValueTag.STRING:
        return vm_render_value_bytes(state, lhs) == vm_render_value_bytes(state, rhs)
    return False


def vm_run(state: VmState) -> VmValue:
    hooks = StdHooks(state)
    if not state.frames:
        entry = state.functions[0]
        locals_init = [VmValue(VmValueTag.NIL, None) for _ in range(entry.param_count + entry.local_count)]
        state.frames.append(VmFrame(0, entry.code_offset, locals_init, 0))

    last = VmValue(VmValueTag.NIL, None)
    while state.frames:
        frame_index = len(state.frames) - 1
        frame = state.frames[frame_index]
        opcode, operands, size = decode_at_pc(state.code, frame.pc)
        if opcode is Opcode.OP_CONST:
            value = vm_value_from_const(state, operands[0], hooks)
            state.stack.append(value)
            frame.pc += size
        elif opcode is Opcode.OP_ADD:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, lhs.value + rhs.value))
            frame.pc += size
        elif opcode is Opcode.OP_SUB:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, lhs.value - rhs.value))
            frame.pc += size
        elif opcode is Opcode.OP_MUL:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, lhs.value * rhs.value))
            frame.pc += size
        elif opcode is Opcode.OP_DIV:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, lhs.value // rhs.value))
            frame.pc += size
        elif opcode is Opcode.OP_MOD:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, lhs.value % rhs.value))
            frame.pc += size
        elif opcode is Opcode.OP_NEG:
            val = state.stack.pop()
            state.stack.append(VmValue(VmValueTag.I64, -val.value))
            frame.pc += size
        elif opcode in (Opcode.OP_CMP_EQ, Opcode.OP_CMP_NE, Opcode.OP_CMP_LT, Opcode.OP_CMP_LE, Opcode.OP_CMP_GT, Opcode.OP_CMP_GE):
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            if opcode is Opcode.OP_CMP_EQ:
                res = vm_equals(state, lhs, rhs)
            elif opcode is Opcode.OP_CMP_NE:
                res = not vm_equals(state, lhs, rhs)
            elif opcode is Opcode.OP_CMP_LT:
                res = lhs.value < rhs.value
            elif opcode is Opcode.OP_CMP_LE:
                res = lhs.value <= rhs.value
            elif opcode is Opcode.OP_CMP_GT:
                res = lhs.value > rhs.value
            else:
                res = lhs.value >= rhs.value
            state.stack.append(VmValue(VmValueTag.BOOL, res))
            frame.pc += size
        elif opcode is Opcode.OP_LOAD_LOCAL:
            idx = operands[0]
            state.stack.append(frame.locals[idx])
            frame.pc += size
        elif opcode is Opcode.OP_STORE_LOCAL:
            idx = operands[0]
            frame.locals[idx] = state.stack.pop()
            frame.pc += size
        elif opcode is Opcode.OP_ALLOC_HEAP:
            kind = operands[0]
            tag = HeapTag.STRING if kind == 0 else HeapTag.ARRAY if kind == 1 else HeapTag.STRUCT
            ref = heap_alloc(state.heap, tag)
            state.stack.append(VmValue(VmValueTag.STRING if tag is HeapTag.STRING else VmValueTag.ARRAY if tag is HeapTag.ARRAY else VmValueTag.STRUCT, ref))
            frame.pc += size
        elif opcode is Opcode.OP_LOAD_FIELD:
            idx = operands[0]
            target = state.stack.pop()
            obj = state.heap[target.value]
            state.stack.append(obj.payload[idx])
            frame.pc += size
        elif opcode is Opcode.OP_STORE_FIELD:
            idx = operands[0]
            value = state.stack.pop()
            target = state.stack.pop()
            obj = state.heap[target.value]
            while len(obj.payload) <= idx:
                obj.payload.append(VmValue(VmValueTag.NIL, None))
            obj.payload[idx] = value
            state.heap[target.value] = obj
            frame.pc += size
        elif opcode is Opcode.OP_CALL:
            fn_index = operands[0]
            func = state.functions[fn_index]
            locals_init = [VmValue(VmValueTag.NIL, None) for _ in range(func.param_count + func.local_count)]
            for arg in range(func.param_count - 1, -1, -1):
                locals_init[arg] = state.stack.pop()
            frame.pc += size
            state.frames[frame_index] = frame
            state.frames.append(VmFrame(fn_index, func.code_offset, locals_init, len(state.stack)))
        elif opcode is Opcode.OP_STD_PRINT or opcode is Opcode.OP_STD_PRINTLN:
            val = state.stack.pop()
            rt = hooks.make_string(vm_render_value_bytes(state, val))
            if opcode is Opcode.OP_STD_PRINTLN:
                hooks.println(rt)
            else:
                hooks.print(rt)
            state.stack.append(val)
            frame.pc += size
        elif opcode is Opcode.OP_STD_MAKE_STRING:
            value = vm_value_from_const(state, operands[0], hooks)
            state.stack.append(value)
            frame.pc += size
        elif opcode is Opcode.OP_STD_CONCAT_STRING:
            rhs = state.stack.pop()
            lhs = state.stack.pop()
            lhs_rt = hooks.make_string(vm_render_value_bytes(state, lhs))
            rhs_rt = hooks.make_string(vm_render_value_bytes(state, rhs))
            merged = hooks.concat_string(lhs_rt, rhs_rt)
            ref = heap_alloc(state.heap, HeapTag.STRING)
            state.heap[ref].payload = list(merged.bytes)
            state.stack.append(VmValue(VmValueTag.STRING, ref))
            frame.pc += size
        elif opcode is Opcode.OP_STD_ARRAY_PUSH:
            value = state.stack.pop()
            arr = state.stack.pop()
            if state.heap[arr.value].tag is not HeapTag.ARRAY:
                raise AssertionError("array_push expects array")
            state.heap[arr.value].payload.append(value)
            hooks.array_push(RtArray(len(state.heap[arr.value].payload), len(state.heap[arr.value].payload), arr.value), value)
            state.stack.append(arr)
            frame.pc += size
        elif opcode is Opcode.OP_STD_ARRAY_GET:
            idx = state.stack.pop()
            arr = state.stack.pop()
            obj = state.heap[arr.value]
            hooks.array_get(RtArray(len(obj.payload), len(obj.payload), arr.value), idx.value)
            state.stack.append(obj.payload[idx.value])
            frame.pc += size
        elif opcode is Opcode.OP_RET:
            last = state.stack.pop()
            state.frames.pop()
            if not state.frames:
                return last
            state.stack.append(last)
        else:
            raise AssertionError(f"unhandled opcode {opcode}")
        if opcode is not Opcode.OP_CALL and opcode is not Opcode.OP_RET:
            state.frames[frame_index] = frame
    return last


def make_chunk(consts: List[Const], code: List[int], functions: List[FunctionEntry] | None = None) -> VmState:
    if functions is None:
        functions = [FunctionEntry(0, 0, len(code), 0, 0)]
    return VmState(consts, functions, code)


class VmOpcodeTest(unittest.TestCase):
    def test_arithmetic_and_comparisons(self) -> None:
        consts = [
            Const(ConstTag.STRING, "demo"),
            Const(ConstTag.I64, 10),
            Const(ConstTag.I64, 3),
            Const(ConstTag.I64, 2),
        ]
        add_code = encode_inst(Opcode.OP_CONST, 1) + encode_inst(Opcode.OP_CONST, 2) + encode_inst(Opcode.OP_ADD) + encode_inst(Opcode.OP_RET)
        state = make_chunk(consts, add_code)
        result = vm_run(state)
        self.assertEqual(result.tag, VmValueTag.I64)
        self.assertEqual(result.value, 13)

        cmp_code = encode_inst(Opcode.OP_CONST, 1) + encode_inst(Opcode.OP_CONST, 2) + encode_inst(Opcode.OP_CMP_GT) + encode_inst(Opcode.OP_RET)
        cmp_state = make_chunk(consts, cmp_code)
        cmp_result = vm_run(cmp_state)
        self.assertEqual(cmp_result.tag, VmValueTag.BOOL)
        self.assertTrue(cmp_result.value)

        neg_code = encode_inst(Opcode.OP_CONST, 2) + encode_inst(Opcode.OP_NEG) + encode_inst(Opcode.OP_RET)
        neg_state = make_chunk(consts, neg_code)
        neg_result = vm_run(neg_state)
        self.assertEqual(neg_result.value, -3)

    def test_calls_and_stack_frames(self) -> None:
        consts = [Const(ConstTag.I64, 42)]
        main_code = encode_inst(Opcode.OP_CONST, 0) + encode_inst(Opcode.OP_CALL, 1) + encode_inst(Opcode.OP_RET)
        callee_offset = len(main_code)
        callee_code = encode_inst(Opcode.OP_LOAD_LOCAL, 0) + encode_inst(Opcode.OP_RET)
        code = main_code + callee_code
        functions = [
            FunctionEntry(0, 0, len(main_code), 0, 0),
            FunctionEntry(0, callee_offset, len(callee_code), 1, 0),
        ]
        state = make_chunk(consts, code, functions)
        result = vm_run(state)
        self.assertEqual(result.tag, VmValueTag.I64)
        self.assertEqual(result.value, 42)

    def test_heap_and_std_array_ops(self) -> None:
        consts = [Const(ConstTag.I64, 7), Const(ConstTag.I64, 1)]
        code = (
            encode_inst(Opcode.OP_ALLOC_HEAP, 1)  # array
            + encode_inst(Opcode.OP_STORE_LOCAL, 0)
            + encode_inst(Opcode.OP_LOAD_LOCAL, 0)
            + encode_inst(Opcode.OP_CONST, 0)
            + encode_inst(Opcode.OP_STORE_FIELD, 0)
            + encode_inst(Opcode.OP_LOAD_LOCAL, 0)
            + encode_inst(Opcode.OP_LOAD_FIELD, 0)
            + encode_inst(Opcode.OP_STORE_LOCAL, 1)
            + encode_inst(Opcode.OP_LOAD_LOCAL, 0)
            + encode_inst(Opcode.OP_LOAD_LOCAL, 1)
            + encode_inst(Opcode.OP_STD_ARRAY_PUSH)
            + encode_inst(Opcode.OP_CONST, 1)
            + encode_inst(Opcode.OP_STD_ARRAY_GET)
            + encode_inst(Opcode.OP_RET)
        )
        functions = [FunctionEntry(0, 0, len(code), 0, 2)]
        state = make_chunk(consts, code, functions)
        result = vm_run(state)
        self.assertEqual(result.tag, VmValueTag.I64)
        self.assertEqual(result.value, 7)

    def test_std_string_instructions_and_stub_validation(self) -> None:
        consts = [
            Const(ConstTag.STRING, "hi"),
            Const(ConstTag.STRING, " there"),
        ]
        code = (
            encode_inst(Opcode.OP_CONST, 0)
            + encode_inst(Opcode.OP_STD_PRINTLN)
            + encode_inst(Opcode.OP_CONST, 0)
            + encode_inst(Opcode.OP_CONST, 1)
            + encode_inst(Opcode.OP_STD_CONCAT_STRING)
            + encode_inst(Opcode.OP_STD_PRINT)
            + encode_inst(Opcode.OP_RET)
        )
        state = make_chunk(consts, code)
        result = vm_run(state)
        self.assertEqual(state.printed, ["hi\n", "hi there"])
        self.assertEqual(vm_render_value_bytes(state, result), list("hi there".encode("utf-8")))

        validation_error = validate_code_bytes(code)
        self.assertEqual(validation_error, "")


if __name__ == "__main__":
    unittest.main()
