# Lyon Error Codes

Lyon error helpers accept caller-defined codes and normalize missing values to
`LYON_E_UNKNOWN`.

Recommended codes:

- `LYON_E_UNKNOWN`: no explicit error code was supplied.
- `LYON_E_CONTEXT`: context construction or merge failure.
- `LYON_E_RESULT`: invalid result conversion.
- `LYON_E_OPTION`: invalid optional value conversion.
- `LYON_E_RECOVERY`: retry or recovery path failed.
- `LYON_E_REPORT`: report aggregation failed.
- `LYON_E_INSTALL`: package loading failed after installation.

Every code should have a stable message and, when possible, a `path` value.
