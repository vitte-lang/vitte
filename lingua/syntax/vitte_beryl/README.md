# vitte_beryl

Lexer, Tokenization, and Bytecode representation for Vitte.

## Features

- Token types and representation
- Lexer for source code tokenization
- Bytecode operations for intermediate representation
- Token stream trait for extensible parsing
- Token buffer for lookahead parsing
- Position tracking (line, column)

## Usage

```vit
use vitte_beryl::{Lexer, Token, TokenType, Bytecode, BytecodeOp};

fn main() {
    let mut lexer = Lexer::new("x = 42".to_string());
    let tokens = lexer.tokenize();
    
    let mut bc = Bytecode::new();
    bc.emit(BytecodeOp::Load(0));
    bc.emit(BytecodeOp::Add);
    bc.emit(BytecodeOp::Return);
}
```
