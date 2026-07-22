# Paris Error Codes

Paris mostly returns structured validation results. The strict reader
`require_option` dies with a stable message when a required option is absent:

- `missing required option --name`

Recommended integration codes for callers:

- `PARIS_E_MISSING_OPTION`: required option is absent.
- `PARIS_E_UNKNOWN_OPTION`: an argument is not present in the allowed option set.
- `PARIS_E_DUPLICATE_SPEC_OPTION`: command spec has duplicate normalized options.
- `PARIS_E_SPEC_NAME`: command spec has no name.
- `PARIS_E_CONFIG_LINE`: config line could not be parsed.
