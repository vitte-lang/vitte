# Deprecation Policy

Vitte is experimental, but deprecation should still be explicit and readable.

## Purpose

The goal of deprecation is to reduce surprise, not just delay removal.

## Minimum Deprecation Flow

1. announce the deprecation in docs or release notes
2. keep a bounded migration path when feasible
3. state the expected removal target if one exists
4. remove the deprecated surface once the window expires

## What Good Deprecation Looks Like

- the old surface is clearly identified
- the replacement is named
- the removal target is visible
- tests and docs reflect the transition

## What To Avoid

- silent removals of documented public behavior
- deprecation without a migration path when a migration path is reasonable
- leaving deprecated wrappers indefinitely without ownership

## Repository Signals

The repository already contains deprecation-related enforcement and metadata in several places.
This policy is the high-level rule that should guide those mechanisms.
