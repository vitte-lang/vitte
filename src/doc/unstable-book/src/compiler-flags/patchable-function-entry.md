# `patchable-function-entry`

--------------------

The `-Z patchable-function-entry=total_nops,prefix_nops` or `-Z patchable-function-entry=total_nops`
compiler flag enables nop padding of function entries with 'total_nops' nops, with
an offset for the entry of the function at 'prefix_nops' nops. In the second form,
'prefix_nops' defaults to 0.

As an illustrative example, `-Z patchable-function-entry=3,2` would produce:

```text
nop
nop
function_label:
nop
//Actual function code begins here
```

This flag is used for hotpatching, especially in the Linux kernel. The flag
arguments are modeled after the `-fpatchable-function-entry` flag as defined
and is intended to provide the same effect.
