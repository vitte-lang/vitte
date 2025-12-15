pub fn utf8_validate(bytes: &[u8]) -> bool {
  // Strict UTF-8 validation using std; correct and simple.
  std::str::from_utf8(bytes).is_ok()
}

// Placeholder: later add normalization/casefold APIs.
pub fn normalize_nfc(bytes: &[u8]) -> Vec<u8> {
  // Stub: return unchanged.
  bytes.to_vec()
}
