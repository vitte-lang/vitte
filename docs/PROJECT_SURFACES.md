# Project Surfaces

Vitte is broad enough that readers can misread the repository as one uniform maturity level.

This document classifies the major public-facing surfaces.

## Core Project

These areas define the center of the project:

- language core policy and guarantees
- compiler and driver pipeline
- main user commands: `parse`, `check`, `build`
- bounded package governance model

If these are unclear, the rest of the project becomes harder to evaluate.

## Official Project Surfaces

These are repository surfaces that are clearly part of the maintained project story:

- documented package families under `docs/`
- official tools under `tools/`
- maintained shell completions
- maintained editor integrations
- contribution, support, and security docs

These are real project surfaces, but not all of them are equally stable.

## Adjacent / Emerging Surfaces

These areas are meaningful, but should be read with more caution:

- VitteOS work and milestone planning
- broader package inventory beyond the bounded public core
- project apps and IDE experiments under `apps/`

These surfaces help show direction and experimentation, but they should not be mistaken for fully mature public guarantees.

## Experimental Surfaces

Examples of areas that should be read as experimental unless docs state otherwise:

- new package families without clear compatibility language
- internal compiler implementation details
- internal package modules
- fast-moving tooling and one-off scripts
- apps intended as repository experiments rather than canonical user paths

## Reading Rule

When in doubt:

1. start with the core project docs
2. check whether a surface has dedicated docs and gates
3. assume `experimental` rather than over-infer stability
