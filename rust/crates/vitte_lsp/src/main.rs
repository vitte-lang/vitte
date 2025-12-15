use std::io::{self, Read};

fn main() {
  // Minimal stub: consume stdin (LSP transport would be stdio) and exit.
  let mut _buf = String::new();
  let _ = io::stdin().read_to_string(&mut _buf);
}
