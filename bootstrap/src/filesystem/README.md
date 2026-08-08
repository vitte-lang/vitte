# bootstrap/src/filesystem

This directory provides the bootstrap compiler filesystem layer. It is a small
C17 API around paths, files, directories, and reader/writer handles.

## Contract

- No dependency on `runtime/*`.
- Errors use `bootstrap/src/api/error.h`.
- Public path buffers are fixed-size and bounded by `VITTE_FS_MAX_PATH`.
- Input strings are never modified.
- Functions return explicit `vitte_status_t` values.
- Caller-owned buffers remain caller-owned.
- Buffers returned by `vitte_fs_read_all_alloc` are freed with `vitte_fs_free`.
- File operations are binary-safe.
- Directory creation is non-destructive.

## Path API

`vitte_fs_path_t` stores normalized paths using `/` separators. The API covers:

- init, copy, and construction from C strings
- join with overflow checks
- basename and dirname
- extension lookup and replacement
- absolute and relative checks
- extension comparison

Hidden files such as `.env` are treated as names without extensions.

## Files

Reading:

- `vitte_fs_read_all` reads into a caller-provided buffer and appends `'\0'`.
- `vitte_fs_read_all_alloc` allocates a buffer, applies `max_file_bytes`, and
  optionally appends `'\0'`.

Writing:

- `vitte_fs_write_all` truncates and writes a complete buffer.
- `vitte_fs_append_all` appends a complete buffer.
- Partial read/write errors fail the operation.

## Directories

- `vitte_fs_create_directory` creates one directory.
- `vitte_fs_create_directories` creates parents recursively.
- `vitte_fs_parent_path` returns the dirname of a path.

The implementation does not remove files or directories.

## Reader And Writer

`vitte_fs_reader_t` and `vitte_fs_writer_t` wrap `FILE *` with tracked path,
byte counters, last error, and safe double close.

Reader lifecycle:

```c
vitte_fs_reader_t reader;
char buffer[64];
size_t count;

vitte_fs_reader_open(&reader, "input.vit");
vitte_fs_reader_read(&reader, buffer, sizeof(buffer), &count);
vitte_fs_reader_close(&reader);
```

Writer lifecycle:

```c
vitte_fs_writer_t writer;
size_t count;

vitte_fs_writer_open(&writer, "out.bin", VITTE_FS_WRITER_TRUNCATE);
vitte_fs_writer_write(&writer, "abc", 3u, &count);
vitte_fs_writer_close(&writer);
```

## Errors

Common codes:

- `VITTE_FS_E_ARGUMENT`
- `VITTE_FS_E_PATH`
- `VITTE_FS_E_OPEN`
- `VITTE_FS_E_READ`
- `VITTE_FS_E_WRITE`
- `VITTE_FS_E_CLOSE`
- `VITTE_FS_E_TOO_LARGE`
- `VITTE_FS_E_MKDIR`
- `VITTE_FS_E_ALLOC`

## Portability

The API is written as C17 and uses `FILE *`, `stat`, and `mkdir` with a small
Windows compatibility branch for `_mkdir`. Temporary paths use `TMPDIR` or
`/tmp`; `vitte_fs_temp_file_path` is deterministic enough for bootstrap tests
but does not guarantee collision-free creation.
