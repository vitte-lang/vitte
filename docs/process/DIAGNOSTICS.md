# vitte/process diagnostics

| Code | Cause | Quick-fix |
|---|---|---|
| VITTE-P0001 | invalid process config | fix required fields and timeout/grace invariants |
| VITTE-P0002 | program not found | install binary or fix `program` path |
| VITTE-P0003 | permission denied | remove denied command or lower privilege requirement |
| VITTE-P0004 | spawn failed | retry spawn and inspect environment |
| VITTE-P0005 | wait failed | retry wait or check process handle state |
| VITTE-P0006 | timeout reached | increase `timeout_ms` and `grace_ms` |
| VITTE-P0007 | terminate/kill failed | verify process running state and signal policy |
| VITTE-P0008 | invalid cwd | set absolute workspace path |
| VITTE-P0009 | invalid env key/value | normalize env as `KEY=VALUE` |
| VITTE-P0010 | capture limit exceeded | increase or clamp `capture_limit` |
| VITTE-P0011 | shell denied by policy | set `allow_shell=false` and pass argv directly |
| VITTE-P0012 | profile denied | use desktop/system profile for process execution |
| VITTE-P0013 | command not allowlisted | add command to `allowlist_commands` |
| VITTE-P0014 | unsafe argument pattern | sanitize args and avoid `sh -c` |
| VITTE-P0015 | background denied | enable `allow_background` explicitly |
