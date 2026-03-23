# Platform Matrix

This page states platform expectations in a direct, non-aspirational way.

## Status Legend

| Status | Meaning |
| --- | --- |
| `credible` | repository docs and workflows make this a real supported path |
| `experimental` | works in some paths, but should not be assumed reliable |
| `unclear` | repository contains related work, but not enough evidence for a stronger claim |

## Current Matrix

| Platform | Status | Notes |
| --- | --- | --- |
| Linux (Ubuntu/Debian) | credible | main CI and install path are clearly documented |
| macOS | credible | documented install path exists and packaging flow is present |
| Windows via WSL2 | credible | recommended path in `README.md` |
| Native Windows | experimental | explicitly described as experimental |
| Arduino / embedded experiments | experimental | repository contains examples and runtime work, but not broad support guarantees |
| VitteOS / kernel targets | experimental | active project direction, not yet a general supported platform promise |

## Reading Rule

Credible does not mean fully stable.
It means the repository provides a serious, repeatable path for contributors and early users.

Experimental means:

- expect setup friction
- expect narrower coverage
- expect faster churn

## Promotion Rule

A platform should move from `experimental` to `credible` only when all of these are true:

1. there is a documented install or bootstrap path
2. there is at least one repeatable CI or scripted validation path
3. common failure modes are documented or diagnosable
4. the public docs describe the platform honestly
