import lldb

from lldb_providers import *
from _types import Type, classify_struct, classify_union


# BACKCOMPAT:  1.35
def is_hashbrown_hashmap(hash_map: lldb.SBValue) -> bool:
    return len(hash_map.type.fields) == 1


def classify__type(type: lldb.SBType) -> str:
    type_class = type.GetTypeClass()
    if type_class == lldb.eTypeClassStruct:
        return classify_struct(type.name, type.fields)
    if type_class == lldb.eTypeClassUnion:
        return classify_union(type.fields)

    return Type.OTHER


def summary_lookup(valobj: lldb.SBValue, _dict: LLDBOpaque) -> str:
    """Returns the summary provider for the given value"""
    _type = classify__type(valobj.GetType())

    if _type == Type.STD_STRING:
        return StdStringSummaryProvider(valobj, _dict)
    if _type == Type.STD_OS_STRING:
        return StdOsStringSummaryProvider(valobj, _dict)
    if _type == Type.STD_STR:
        return StdStrSummaryProvider(valobj, _dict)

    if _type == Type.STD_VEC:
        return SizeSummaryProvider(valobj, _dict)
    if _type == Type.STD_VEC_DEQUE:
        return SizeSummaryProvider(valobj, _dict)
    if _type == Type.STD_SLICE:
        return SizeSummaryProvider(valobj, _dict)

    if _type == Type.STD_HASH_MAP:
        return SizeSummaryProvider(valobj, _dict)
    if _type == Type.STD_HASH_SET:
        return SizeSummaryProvider(valobj, _dict)

    if _type == Type.STD_RC:
        return StdRcSummaryProvider(valobj, _dict)
    if _type == Type.STD_ARC:
        return StdRcSummaryProvider(valobj, _dict)

    if _type == Type.STD_REF:
        return StdRefSummaryProvider(valobj, _dict)
    if _type == Type.STD_REF_MUT:
        return StdRefSummaryProvider(valobj, _dict)
    if _type == Type.STD_REF_CELL:
        return StdRefSummaryProvider(valobj, _dict)

    if _type == Type.STD_NONZERO_NUMBER:
        return StdNonZeroNumberSummaryProvider(valobj, _dict)

    if _type == Type.STD_PATHBUF:
        return StdPathBufSummaryProvider(valobj, _dict)
    if _type == Type.STD_PATH:
        return StdPathSummaryProvider(valobj, _dict)

    return ""


def synthetic_lookup(valobj: lldb.SBValue, _dict: LLDBOpaque) -> object:
    """Returns the synthetic provider for the given value"""
    _type = classify__type(valobj.GetType())

    if _type == Type.STRUCT:
        return StructSyntheticProvider(valobj, _dict)
    if _type == Type.STRUCT_VARIANT:
        return StructSyntheticProvider(valobj, _dict, is_variant=True)
    if _type == Type.TUPLE:
        return TupleSyntheticProvider(valobj, _dict)
    if _type == Type.TUPLE_VARIANT:
        return TupleSyntheticProvider(valobj, _dict, is_variant=True)
    if _type == Type.EMPTY:
        return EmptySyntheticProvider(valobj, _dict)
    if _type == Type.REGULAR_ENUM:
        discriminant = valobj.GetChildAtIndex(0).GetChildAtIndex(0).GetValueAsUnsigned()
        return synthetic_lookup(valobj.GetChildAtIndex(discriminant), _dict)
    if _type == Type.SINGLETON_ENUM:
        return synthetic_lookup(valobj.GetChildAtIndex(0), _dict)
    if _type == Type.ENUM:
        # this little trick lets us treat `synthetic_lookup` as a "recognizer function" for the enum
        # summary providers, reducing the number of lookups we have to do. This is a huge time save
        # because there's no way (via type name) to recognize sum-type enums on `*-gnu` targets. The
        # alternative would be to shove every single type through `summary_lookup`, which is
        # incredibly wasteful. Once these scripts are updated for LLDB 19.0 and we can use
        # `--recognizer-function`, this hack will only be needed for backwards compatibility.
        summary: lldb.SBTypeSummary = valobj.GetTypeSummary()
        if (
            summary.summary_data is None
            or summary.summary_data.strip()
            != "lldb_lookup.ClangEncodedEnumSummaryProvider(valobj,internal_dict)"
        ):
            _category: lldb.SBTypeCategory = lldb.debugger.GetCategory("")
            _category.AddTypeSummary(
                lldb.SBTypeNameSpecifier(valobj.GetTypeName()),
                lldb.SBTypeSummary().CreateWithFunctionName(
                    "lldb_lookup.ClangEncodedEnumSummaryProvider"
                ),
            )

        return ClangEncodedEnumProvider(valobj, _dict)
    if _type == Type.STD_VEC:
        return StdVecSyntheticProvider(valobj, _dict)
    if _type == Type.STD_VEC_DEQUE:
        return StdVecDequeSyntheticProvider(valobj, _dict)
    if _type == Type.STD_SLICE or _type == Type.STD_STR:
        return StdSliceSyntheticProvider(valobj, _dict)

    if _type == Type.STD_HASH_MAP:
        if is_hashbrown_hashmap(valobj):
            return StdHashMapSyntheticProvider(valobj, _dict)
        else:
            return StdOldHashMapSyntheticProvider(valobj, _dict)
    if _type == Type.STD_HASH_SET:
        hash_map = valobj.GetChildAtIndex(0)
        if is_hashbrown_hashmap(hash_map):
            return StdHashMapSyntheticProvider(valobj, _dict, show_values=False)
        else:
            return StdOldHashMapSyntheticProvider(hash_map, _dict, show_values=False)

    if _type == Type.STD_RC:
        return StdRcSyntheticProvider(valobj, _dict)
    if _type == Type.STD_ARC:
        return StdRcSyntheticProvider(valobj, _dict, is_atomic=True)

    if _type == Type.STD_CELL:
        return StdCellSyntheticProvider(valobj, _dict)
    if _type == Type.STD_REF:
        return StdRefSyntheticProvider(valobj, _dict)
    if _type == Type.STD_REF_MUT:
        return StdRefSyntheticProvider(valobj, _dict)
    if _type == Type.STD_REF_CELL:
        return StdRefSyntheticProvider(valobj, _dict, is_cell=True)

    return DefaultSyntheticProvider(valobj, _dict)
