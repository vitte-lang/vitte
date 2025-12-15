use std::io::{self, Read};

fn main() {
  let mut input = String::new();
  io::stdin().read_to_string(&mut input).ok();

  // Placeholder formatter:
  // - strips trailing whitespace
  // - preserves line order
  let mut out = String::new();
  for line in input.lines() {
    out.push_str(line.trim_end());
    out.push('\n');
  }

  // Optional: if it parses as MUF, you can emit normalized form later.
  // let _ = vitte_muf::muf_to_json_bytes(out.as_bytes());

  print!("{out}");
}
