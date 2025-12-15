#[cfg(feature = "regex_crate")]
pub fn is_match(hay: &[u8], pat: &[u8]) -> bool {
  let hay = match std::str::from_utf8(hay) { Ok(s) => s, Err(_) => return false };
  let pat = match std::str::from_utf8(pat) { Ok(s) => s, Err(_) => return false };
  match regex::Regex::new(pat) {
    Ok(re) => re.is_match(hay),
    Err(_) => false,
  }
}

#[cfg(not(feature = "regex_crate"))]
pub fn is_match(hay: &[u8], pat: &[u8]) -> bool {
  // Minimal implementation: substring search.
  if pat.is_empty() { return true; }
  hay.windows(pat.len()).any(|w| w == pat)
}
