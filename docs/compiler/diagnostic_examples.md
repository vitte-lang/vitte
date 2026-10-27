# Diagnostic Examples

These examples are intentionally small. Each invalid example shows the local
shape that should trigger a diagnostic; each corrected example shows the same
intent written as valid Vitte.

## Parser

Invalid:

```vitte
proc main() -> int {
  return 0
```

Corrected:

```vitte
proc main() -> int {
  return 0
}
```

## Resolver

Invalid:

```vitte
proc main() -> int {
  return total
}
```

Corrected:

```vitte
proc main() -> int {
  let total: int = 0;
  return total;
}
```

## Typeck

Invalid:

```vitte
proc main() -> int {
  return "zero";
}
```

Corrected:

```vitte
proc main() -> int {
  return 0;
}
```

## Borrowck

Invalid:

```vitte
proc main() -> int {
  let value: string = "v";
  let moved: string = value;
  return len(value);
}
```

Corrected:

```vitte
proc main() -> int {
  let value: string = "v";
  return len(value);
}
```

## Linker

Invalid:

```vitte
extern "C" {
  proc missing_symbol() -> int;
}

proc main() -> int {
  return missing_symbol();
}
```

Corrected:

```vitte
proc missing_symbol() -> int {
  return 0;
}

proc main() -> int {
  return missing_symbol();
}
```
