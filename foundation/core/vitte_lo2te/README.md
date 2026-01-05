# vitte_lo2te

Error reporting and logging infrastructure for the Vitte compiler.

## Features

- Error level classification (Debug, Info, Warning, Error, Critical)
- Error reporting with codes and messages
- Logging backend trait for extensibility
- Context attachment for errors

## Usage

```vit
use vitte_lo2te::{Logger, ErrorReport, ErrorLevel};

fn main() {
    let logger = Logger::new(true);
    let error = ErrorReport::new(
        ErrorLevel::Error,
        42,
        "Compilation failed".to_string()
    ).with_context("At parsing phase".to_string());
    logger.report(error);
}
```
