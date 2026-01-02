# stdlib/cli

Command-line utilities for Vitte standard library.

Scope:
- argument parsing (flags, subcommands)
- ANSI styling + terminal controls
- prompts (confirm/select/input)
- progress (spinners, bars)
- tables + formatting helpers
- completion scripts generator
- structured logging for CLI tools

Notes:
- Public surface is designed to be stable and host-friendly.
- Terminal IO implementation uses runtime/cli backends (where applicable).
