use vitte_encoding::Encoding;

/// Label WHATWG approximatif pour l’encodage détecté.
/// Ici on se limite à quelques cas communs et noms stables.
/// Utile quand un shell ou un fichier sidecar veut un label texte.
pub fn best_label(enc: Encoding) -> &'static str {
    match enc {
        Encoding::Utf8 => "utf-8",
        Encoding::Utf16LE => "utf-16le",
        Encoding::Utf16BE => "utf-16be",
        Encoding::Latin1 => "iso-8859-1",
        Encoding::Ascii => "us-ascii",
    }
}