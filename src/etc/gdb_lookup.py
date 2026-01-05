import gdb
import gdb.printing
import re

from gdb_providers import *
from _types import *


_gdb_version_matched = re.search("([0-9]+)\\.([0-9]+)", gdb.VERSION)
gdb_version = (
    [int(num) for num in _gdb_version_matched.groups()] if _gdb_version_matched else []
)


def register_printers(objfile):
    objfile.pretty_printers.append(printer)


# BACKCOMPAT:  1.35
def is_hashbrown_hashmap(hash_map):
    return len(hash_map.type.fields()) == 1


def classify__type(type):
    type_class = type.code
    if type_class == gdb.TYPE_CODE_STRUCT:
        return classify_struct(type.tag, type.fields())
    if type_class == gdb.TYPE_CODE_UNION:
        return classify_union(type.fields())

    return Type.OTHER


def check_enum_discriminant(valobj):
    content = valobj[valobj.type.fields()[0]]
    fields = content.type.fields()
    if len(fields) > 1:
        discriminant = int(content[fields[0]]) + 1
        if discriminant > len(fields):
            # invalid discriminant
            return False
    return True


# Helper for enum printing that checks the discriminant.  Only used in
# older gdb.
def enum_provider(valobj):
    if check_enum_discriminant(valobj):
        return EnumProvider(valobj)
    return None


# Helper to handle both old and new hash maps.
def hashmap_provider(valobj):
    if is_hashbrown_hashmap(valobj):
        return StdHashMapProvider(valobj)
    else:
        return StdOldHashMapProvider(valobj)


# Helper to handle both old and new hash sets.
def hashset_provider(valobj):
    hash_map = valobj[valobj.type.fields()[0]]
    if is_hashbrown_hashmap(hash_map):
        return StdHashMapProvider(valobj, show_values=False)
    else:
        return StdOldHashMapProvider(hash_map, show_values=False)


class PrintByType(gdb.printing.SubPrettyPrinter):
    def __init__(self, name, provider):
        super(PrintByType, self).__init__(name)
        self.provider = provider

    def __call__(self, val):
        if self.enabled:
            return self.provider(val)
        return None


class PrettyPrinter(gdb.printing.PrettyPrinter):
    def __init__(self, name):
        super(PrettyPrinter, self).__init__(name, [])
        self.type_map = {}

    def add(self, _type, provider):
        # Just use the _type as the name.
        printer = PrintByType(_type, provider)
        self.type_map[_type] = printer
        self.subprinters.append(printer)

    def __call__(self, valobj):
        _type = classify__type(valobj.type)
        if _type in self.type_map:
            return self.type_map[_type](valobj)
        return None


printer = PrettyPrinter("")
# use enum provider only for GDB <7.12
if gdb_version[0] < 7 or (gdb_version[0] == 7 and gdb_version[1] < 12):
    printer.add(Type.ENUM, enum_provider)
printer.add(Type.STD_STRING, StdStringProvider)
printer.add(Type.STD_OS_STRING, StdOsStringProvider)
printer.add(Type.STD_STR, StdStrProvider)
printer.add(Type.STD_SLICE, StdSliceProvider)
printer.add(Type.STD_VEC, StdVecProvider)
printer.add(Type.STD_VEC_DEQUE, StdVecDequeProvider)
printer.add(Type.STD_BTREE_SET, StdBTreeSetProvider)
printer.add(Type.STD_BTREE_MAP, StdBTreeMapProvider)
printer.add(Type.STD_HASH_MAP, hashmap_provider)
printer.add(Type.STD_HASH_SET, hashset_provider)
printer.add(Type.STD_RC, StdRcProvider)
printer.add(Type.STD_ARC, lambda valobj: StdRcProvider(valobj, is_atomic=True))

printer.add(Type.STD_CELL, StdCellProvider)
printer.add(Type.STD_REF, StdRefProvider)
printer.add(Type.STD_REF_MUT, StdRefProvider)
printer.add(Type.STD_REF_CELL, StdRefCellProvider)

printer.add(Type.STD_NONZERO_NUMBER, StdNonZeroNumberProvider)
