# Compiler Build Directory

This directory is created during the build process and contains:

- Object files (`.o` / `.obj`)
- Executable binaries (`vittec`, `vittec.exe`)
- Build artifacts
- CMake cache and configuration

## Build Output Structure

Typical build outputs:
```
build/
├── debug/
│   ├── vittec          # Debug executable
│   ├── CMakeFiles/     # CMake cache
│   └── *.o             # Object files
└── release/
    ├── vittec          # Release executable
    └── *.o             # Object files
```

## Building

From the project root:

### Debug build:
```bash
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
make
```

### Release build:
```bash
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make
```

## Clean Build

```bash
rm -rf build/
```
