# Week 7-8 - Bug Triage

## Severity policy
- Critical: crash, data corruption, security fault
- Major: correctness regression, broken core workflow
- Minor: ergonomics, docs, non-blocking UX issues

## Ticket inventory
- BETA-101 (Critical): async executor queue starvation under burst scheduling
- BETA-102 (Major): FFI unresolved symbol path returns ambiguous error
- BETA-103 (Minor): reflection module docs mismatch on generic metadata
- BETA-104 (Minor): profiler hotspot ordering not deterministic
- BETA-105 (Minor): package registry mock URL not shown in logs

## Current triage snapshot
- Critical: 0 open (BETA-101 closed)
- Major: 0 open (BETA-102 closed)
- Minor: 3 open (BETA-103/104/105)

## Rule
- All Critical/Major must be closed before release.
