

#![deny(missing_docs)]
//! vitte-policy — moteur de politiques et règles pour Vitte
//!
//! Objectifs :
//! - Définition de règles déclaratives (avec conditions, patterns).
//! - Validation de documents JSON contre schémas (optionnel).
//! - Évaluation de politiques avec contexte runtime.
//! - Gestion des erreurs et profils de sécurité.
//!
//! Features :
//! - `serde` : sérialisation des règles et politiques.
//! - `jsonschema` : validation via schémas JSON.
//! - `regex` : correspondances par expressions régulières.
//! - `time` : règles temporelles (horodatages, TTL).

use thiserror::Error;

/// Erreurs du moteur de politiques.
#[derive(Debug, Error)]
pub enum PolicyError {
    #[error("JSON error: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation JSON.
    Json(#[from] serde_json::Error),

    #[error("Regex error: {0}")]
    #[cfg(feature = "regex")]
    Regex(#[from] regex::Error),

    #[error("Validation failed: {0}")]
    /// Échec de validation d'une règle ou d'une politique (détails dans la chaîne).
    Validation(String),

    #[error("Other: {0}")]
    /// Erreur générique non catégorisée.
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, PolicyError>;

/// Représente une règle élémentaire.
#[cfg_attr(feature="serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Debug, Clone, PartialEq)]
pub struct Rule {
    /// Identifiant unique de la règle.
    pub id: String,
    /// Expression conditionnelle sous forme de chaîne.
    pub condition: String,
    /// Message ou label associé.
    pub message: Option<String>,
}

/// Une politique est un ensemble de règles.
#[cfg_attr(feature="serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Debug, Clone, PartialEq, Default)]
pub struct Policy {
    /// Nom de la politique.
    pub name: String,
    /// Ensemble de règles.
    pub rules: Vec<Rule>,
}

impl Policy {
    /// Évalue la politique sur un document JSON brut.
    pub fn evaluate(&self, doc: &serde_json::Value) -> Result<Vec<String>> {
        let mut msgs = Vec::new();
        for rule in &self.rules {
            if eval_condition(&rule.condition, doc)? {
                if let Some(m) = &rule.message {
                    msgs.push(m.clone());
                } else {
                    msgs.push(format!("rule {} matched", rule.id));
                }
            }
        }
        Ok(msgs)
    }
}

/// Évalue une condition rudimentaire.
/// Implémentation simple: vérifie existence d'un champ ou regex match si activé.
fn eval_condition(cond: &str, doc: &serde_json::Value) -> Result<bool> {
    if let Some(v) = doc.get(cond) {
        return Ok(!v.is_null());
    }
    #[cfg(feature="regex")]
    {
        let re = regex::Regex::new(cond)?;
        if re.is_match(&doc.to_string()) { return Ok(true); }
    }
    Ok(false)
}

/// Validation JSON via schéma (si activé).
#[cfg(feature="jsonschema")]
pub mod validation {
    use super::*;
    use jsonschema::{JSONSchema, Draft};
    pub fn validate(schema: &serde_json::Value, instance: &serde_json::Value) -> Result<()> {
        let compiled = JSONSchema::options().with_draft(Draft::Draft7).compile(schema)
            .map_err(|e| PolicyError::Validation(format!("invalid schema: {e}")))?;
        let result = compiled.validate(instance);
        if let Err(errors) = result {
            let msgs: Vec<String> = errors.map(|e| e.to_string()).collect();
            return Err(PolicyError::Validation(msgs.join("; ")));
        }
        Ok(())
    }
}

/// Règles temporelles si `time`.
#[cfg(feature="time")]
pub mod temporal {
    use super::*;
    use time::OffsetDateTime;
    /// Vérifie qu'une date ISO8601 est encore valide avec TTL secondes.
    pub fn not_expired(date: &str, ttl_secs: i64) -> Result<bool> {
        let dt = OffsetDateTime::parse(date, &time::format_description::well_known::Rfc3339)
            .map_err(|e| PolicyError::Other(format!("parse time: {e}")))?;
        let now = OffsetDateTime::now_utc();
        Ok((now - dt).whole_seconds() <= ttl_secs)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn eval_field_condition() {
        let pol = Policy { name: "t".into(), rules: vec![ Rule { id:"r1".into(), condition:"foo".into(), message:Some("hit".into()) } ]};
        let doc = json!({"foo": 1});
        let res = pol.evaluate(&doc).unwrap();
        assert_eq!(res, vec!["hit"]);
    }

    #[cfg(feature="regex")]
    #[test]
    fn regex_cond() {
        let r = eval_condition("foo.*", &json!({"foo":123})).unwrap();
        assert!(r);
    }

    #[cfg(feature="jsonschema")]
    #[test]
    fn validate_schema() {
        let schema = json!({"type":"object","properties":{"x":{"type":"integer"}},"required":["x"]});
        let inst = json!({"x":5});
        assert!(validation::validate(&schema, &inst).is_ok());
    }

    #[cfg(feature="time")]
    #[test]
    fn temporal_not_expired() {
        use time::OffsetDateTime;
        let now = OffsetDateTime::now_utc();
        let s = now.format(&time::format_description::well_known::Rfc3339).unwrap();
        assert!(temporal::not_expired(&s, 10).unwrap());
    }
}