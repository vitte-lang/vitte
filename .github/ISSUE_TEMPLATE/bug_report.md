name: Bug report
description: Report a reproducible problem
labels: [bug]
body:
  - type: textarea
    id: description
    attributes:
      label: What happened?
      description: Clear description of the problem and what you expected.
    validations:
      required: true
  - type: textarea
    id: steps
    attributes:
      label: Steps to reproduce
      description: Minimal code or commands to reproduce.
      placeholder: |
        1. ...
        2. ...
  - type: input
    id: version
    attributes:
      label: Version / commit
      description: e.g. v0.1.0 or git SHA
  - type: input
    id: platform
    attributes:
      label: Platform
      description: OS/arch and toolchain (rustc -Vv)
