# Release Doctor

status: **fail**

| check | status | report |
|---|---|---|
| contracts_lockfiles | fail | - |
| contracts_dashboard | fail | target/reports/contracts_dashboard.md |
| reports_index | ok | target/reports/index.json |
| security_hardening | fail | target/reports/security_hardening.json |
| security_baseline_diff | fail | - |
| perf_robust | ok | - |
| perf_budget | ok | - |
| docs_sync | ok | - |
| plugin_abi_compat | fail | - |
| plugin_manifest | fail | - |
| plugin_sandbox | fail | - |
| plugin_binary_abi | ok | target/reports/plugin_binary_abi.json |
| repro_report | ok | target/reports/repro.json |

## Failing Reports
- target/reports/contracts_dashboard.md
- target/reports/security_hardening.json
