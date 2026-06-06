# Diagnostics

- Source of truth: error catalog in `vittec0.seed` (`error_catalog_data`).
- Ad-hoc diagnostics are rejected.
- Formats:
  - Text: `[vittec0][error] CODE: message at file:line:col`
  - JSON: includes `code,severity,phase,file,start/end,message,labels,notes,helps`.
