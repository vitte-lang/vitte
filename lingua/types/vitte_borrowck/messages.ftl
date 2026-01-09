## File: C:\Users\gogin\Documents\GitHub\vitte\lingua\types\vitte_borrowck\messages.ftl
## Vitte — borrow checker diagnostics/messages (MAX)
##
## Conventions:
## - vitte-borrowck-<severity>-<topic>[-<detail>]
## - Variables: { $name }, { $place }, { $detail }, { $span }
## - Keep messages concise and actionable; detailed context goes in notes.

# -----------------------------------------------------------------------------
# Generic
# -----------------------------------------------------------------------------

vitte-borrowck-kind = borrowck

vitte-borrowck-help-intro =
    borrow checker rejected this program due to invalid aliasing or move usage

vitte-borrowck-help-fix =
    consider cloning, borrowing immutably, or restructuring to shorten the borrow

# -----------------------------------------------------------------------------
# Use-after-move / moves
# -----------------------------------------------------------------------------

vitte-borrowck-e-use-after-move =
    use of moved value{ $place }

vitte-borrowck-note-move-happened-here =
    value was moved here

vitte-borrowck-e-double-move =
    value was moved more than once{ $place }

vitte-borrowck-e-move-while-borrowed =
    cannot move value while it is borrowed{ $place }

vitte-borrowck-note-active-borrow-here =
    borrow is active here

# -----------------------------------------------------------------------------
# Aliasing rules
# -----------------------------------------------------------------------------

vitte-borrowck-e-mutable-alias =
    cannot borrow value as mutable more than once at a time{ $place }

vitte-borrowck-e-write-while-shared-borrow =
    cannot write to value while it is immutably borrowed{ $place }

vitte-borrowck-note-shared-borrow-here =
    immutable borrow is active here

# -----------------------------------------------------------------------------
# Lifetime / ownership region
# -----------------------------------------------------------------------------

vitte-borrowck-e-borrow-outlives-owner =
    borrowed value does not live long enough{ $place }

vitte-borrowck-note-owner-dropped-here =
    owner is dropped here

# -----------------------------------------------------------------------------
# Warnings
# -----------------------------------------------------------------------------

vitte-borrowck-w-unused-borrow =
    borrow is never used{ $place }

vitte-borrowck-w-redundant-borrow =
    redundant borrow{ $place }

# -----------------------------------------------------------------------------
# Internal / debug
# -----------------------------------------------------------------------------

vitte-borrowck-e-internal =
    internal borrow checker error: { $detail }

vitte-borrowck-trace =
    trace: { $detail }
