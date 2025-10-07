/// Erreurs possibles de vérification.
use thiserror::Error;
#[derive(Debug, Error)]
pub enum LtoError {
    /// Profil Cargo inattendu (ex: `CARGO_PROFILE` ne correspond pas au mode attendu).
    #[error("profil inattendu: {0}")]
    UnexpectedProfile(String),
    /// Configuration incohérente détectée (ex: feature LTO activée mais profil non aligné).
    #[error("configuration incohérente: {0}")]
    Incoherent(String),
    /// Valeur de variable d'environnement invalide ou non reconnue.
    #[error("variable d'environnement invalide: {0}")]
    BadEnv(String),
}