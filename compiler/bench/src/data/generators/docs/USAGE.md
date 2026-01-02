# Usage

## Generate CI fixtures
- `python3 driver.py --preset presets/ci.json --generated ../generated`
- `python3 validate_dataset.py --root ../generated`

## Generate heavier local fixtures
- `python3 driver.py --preset presets/dev_large.json --generated ../generated`

## Refresh fuzz heavyweight fixtures (optional)
- `python3 driver.py --preset presets/ci.json --generated ../generated --fuzz ../fuzz --refresh-fuzz`
- `python3 driver.py --preset presets/dev_large.json --generated ../generated --fuzz ../fuzz --refresh-fuzz`

## Validate presets/manifests (structural)
- `python3 tools/validate_json_schema.py --kind preset --file presets/ci.json`
- `python3 tools/validate_json_schema.py --kind manifest --file ../generated/meta/manifest.json`

## Diff manifests
- `python3 tools/manifest_diff.py ../generated/meta/manifest.json ../generated/meta/manifest.json`

## Normalize LF newlines
- `python3 tools/normalize_lf.py --root ../generated`

## Smoke tests
- `python3 tests/smoke.py`
