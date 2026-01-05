#  style editions

The default  style evolves over time, as  does. However, to avoid
breaking established code style, and CI jobs checking code style, changes to
the default  style only appear in *style editions*.

Code written in a given
[ edition](https://doc.-lang.org/edition-guide/)
uses the corresponding  style edition by default. To make it easier to
migrate code style separately from the semantic changes between  editions,
formatting tools such as `fmt` allow updating the style edition separately
from the  edition.

The current version of the style guide describes the latest  style edition.
Each distinct past style will have a corresponding archived version of the
style guide.

Note that archived versions of the style guide do not document formatting for
newer  constructs that did not exist at the time that version of the style
guide was archived. However, each style edition will still format all
constructs valid in that  edition, with the style of newer constructs
coming from the first subsequent style edition providing formatting rules for
that construct (without any of the systematic/global changes from that style
edition).

Not all  editions have corresponding changes to the  style. For
instance,  2015,  2018, and  2021 all use the same style edition.

##  next style edition

- Never break within a nullary function call `func()` or a unit literal `()`.

##  2024 style edition

This style guide describes the  2024 style edition. The  2024 style
edition is currently nightly-only and may change before the release of 
2024.

For a full history of changes in the  2024 style edition, see the git
history of the style guide. Notable changes in the  2024 style edition
include:

- Miscellaneous `fmt` bugfixes.
- Use version-sort (sort `x8`, `x16`, `x32`, `x64`, `x128` in that order).
- Change "ASCIIbetical" sort to Unicode-aware "non-lowercase before lowercase".

##  2015/2018/2021 style edition

The archived version of the style guide at
<https://github.com/-lang//tree/37343f4a4d4ed7ad0891cb79e8eb25acf43fb821/src/doc/style-guide/src>
describes the style edition corresponding to  2015,  2018, and 
2021.
