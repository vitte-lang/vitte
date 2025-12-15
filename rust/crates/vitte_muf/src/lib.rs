use vitte_common::err::VitteErr;

fn escape_json_str(s: &str) -> String {
  let mut out = String::with_capacity(s.len() + 8);
  for ch in s.chars() {
    match ch {
      '"' => out.push_str("\\""),
      '\\' => out.push_str("\\\\"),
      '\n' => out.push_str("\\n"),
      '\r' => out.push_str("\\r"),
      '\t' => out.push_str("\\t"),
      c if c.is_control() => {
        out.push_str(&format!("\\u{:04x}", c as u32));
      }
      c => out.push(c),
    }
  }
  out
}

/// MUF (toy) format:
///   key = value
/// lines starting with # are comments; empty lines ignored
/// Output: JSON object with string values.
pub fn muf_to_json_bytes(input: &[u8]) -> Result<Vec<u8>, VitteErr> {
  let s = std::str::from_utf8(input).map_err(|_| VitteErr::Parse)?;
  let mut pairs: Vec<(String, String)> = Vec::new();

  for (lineno, line) in s.lines().enumerate() {
    let line = line.trim();
    if line.is_empty() || line.starts_with('#') { continue; }
    let Some(eq) = line.find('=') else { return Err(VitteErr::Parse); };
    let (k, v) = line.split_at(eq);
    let k = k.trim();
    let v = v[1..].trim(); // skip '='
    if k.is_empty() { return Err(VitteErr::Parse); }

    // Allow quoted strings "..."
    let val = if v.starts_with('"') && v.ends_with('"') && v.len() >= 2 {
      &v[1..v.len()-1]
    } else {
      v
    };

    pairs.push((k.to_string(), val.to_string()));
    let _ = lineno;
  }

  let mut json = String::from("{");
  for (i, (k, v)) in pairs.iter().enumerate() {
    if i != 0 { json.push(','); }
    json.push('"');
    json.push_str(&escape_json_str(k));
    json.push_str("\":\"");
    json.push_str(&escape_json_str(v));
    json.push('"');
  }
  json.push('}');
  Ok(json.into_bytes())
}
