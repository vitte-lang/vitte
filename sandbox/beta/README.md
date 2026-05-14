# Beta Sandbox Environment

## Run
```bash
cd sandbox/beta
docker compose up -d --build
docker exec -it vitte-beta bash
```

## Suggested git branch isolation
```bash
git checkout -b beta/<tester-id>
```

## Minimal validation flow inside sandbox
```bash
bin/vitte check src/vitte/stdlib/ffi/ffi.vitl
bin/vitte check src/vitte/stdlib/async/executor.vitl
bin/vitte check src/vitte/stdlib/reflection/reflection.vitl
```
