//! vitte-args — CLI argument parser stub.
//!
//! The original crate offers a fully fledged command-line parser. This stub
//! keeps the key data structures (`Arg`, `Spec`, `Parser`, `Matches`) so that
//! other crates in the workspace may continue to compile while the real parser
//! is under construction. Parsing routines return empty results or
//! `ParseError::Unsupported`.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::borrow::Cow;
use alloc::string::String;
use alloc::vec::Vec;

/// Nature of an argument in the specification.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ArgKind {
    /// Flag (no value).
    Flag,
    /// Option with value.
    Opt,
    /// Positional argument.
    Pos,
}

/// Cardinality of an argument.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Arity {
    /// Exactly one value.
    One,
    /// Optional value.
    ZeroOrOne,
    /// Variadic values.
    Many,
}

impl Default for Arity {
    fn default() -> Self {
        Arity::One
    }
}

/// Argument specification used by the stub.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Arg {
    pub name: String,
    pub kind: ArgKind,
    pub short: Option<char>,
    pub long: Option<String>,
    pub help: Option<String>,
    pub value_name: Option<String>,
    pub required: bool,
    pub arity: Arity,
    pub env: Option<String>,
    pub default: Option<String>,
    pub choices: Vec<String>,
    pub repeatable: bool,
    pub hidden: bool,
    pub deprecated: Option<String>,
    pub aliases: Vec<String>,
    pub requires: Vec<String>,
    pub conflicts: Vec<String>,
}

impl Default for Arg {
    fn default() -> Self {
        Self {
            name: String::new(),
            kind: ArgKind::Flag,
            short: None,
            long: None,
            help: None,
            value_name: None,
            required: false,
            arity: Arity::ZeroOrOne,
            env: None,
            default: None,
            choices: Vec::new(),
            repeatable: false,
            hidden: false,
            deprecated: None,
            aliases: Vec::new(),
            requires: Vec::new(),
            conflicts: Vec::new(),
        }
    }
}

impl Arg {
    /// Creates a flag argument.
    pub fn flag(name: impl Into<String>) -> Self {
        Self {
            kind: ArgKind::Flag,
            name: name.into(),
            arity: Arity::ZeroOrOne,
            ..Self::default()
        }
    }

    /// Creates an option argument.
    pub fn opt(name: impl Into<String>) -> Self {
        Self { kind: ArgKind::Opt, name: name.into(), arity: Arity::One, ..Self::default() }
    }

    /// Creates a positional argument.
    pub fn pos(name: impl Into<String>) -> Self {
        Self { kind: ArgKind::Pos, name: name.into(), arity: Arity::One, ..Self::default() }
    }

    /// Builder helpers mirroring the original API.
    pub fn short(mut self, c: char) -> Self {
        self.short = Some(c);
        self
    }
    pub fn long(mut self, l: &str) -> Self {
        self.long = Some(l.to_string());
        self
    }
    pub fn help(mut self, h: &str) -> Self {
        self.help = Some(h.to_string());
        self
    }
    pub fn value_name(mut self, n: &str) -> Self {
        self.value_name = Some(n.to_string());
        self
    }
    pub fn required(mut self, r: bool) -> Self {
        self.required = r;
        self
    }
    pub fn arity(mut self, a: Arity) -> Self {
        self.arity = a;
        self
    }
    pub fn env(mut self, var: &str) -> Self {
        self.env = Some(var.to_string());
        self
    }
    pub fn with_default(mut self, v: &str) -> Self {
        self.default = Some(v.to_string());
        self
    }
    pub fn choices<I: IntoIterator<Item = String>>(mut self, vals: I) -> Self {
        self.choices = vals.into_iter().collect();
        self
    }
    pub fn repeatable(mut self, r: bool) -> Self {
        self.repeatable = r;
        self
    }
    pub fn hidden(mut self, h: bool) -> Self {
        self.hidden = h;
        self
    }
    pub fn deprecated(mut self, msg: &str) -> Self {
        self.deprecated = Some(msg.to_string());
        self
    }
    pub fn alias(mut self, a: &str) -> Self {
        self.aliases.push(a.to_string());
        self
    }
    pub fn requires(mut self, name: impl Into<String>) -> Self {
        self.requires.push(name.into());
        self
    }
    pub fn conflicts_with(mut self, name: impl Into<String>) -> Self {
        self.conflicts.push(name.into());
        self
    }
}

/// CLI specification.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Spec {
    pub bin: String,
    pub about: Option<String>,
    pub version: Option<String>,
    pub footer: Option<String>,
    pub args: Vec<Arg>,
    pub subcommands: Vec<Spec>,
    pub auto_help: bool,
    pub auto_version: bool,
}

impl Spec {
    /// Creates a new specification with default settings.
    pub fn new(bin: &str) -> Self {
        Self { bin: bin.to_string(), auto_help: true, auto_version: true, ..Self::default() }
    }

    pub fn about(mut self, text: &str) -> Self {
        self.about = Some(text.to_string());
        self
    }
    pub fn version(mut self, v: &str) -> Self {
        self.version = Some(v.to_string());
        self
    }
    pub fn footer(mut self, f: &str) -> Self {
        self.footer = Some(f.to_string());
        self
    }
    pub fn arg(mut self, arg: Arg) -> Self {
        self.args.push(arg);
        self
    }
    pub fn subcommand(mut self, cmd: Spec) -> Self {
        self.subcommands.push(cmd);
        self
    }
    pub fn auto_help(mut self, on: bool) -> Self {
        self.auto_help = on;
        self
    }
    pub fn auto_version(mut self, on: bool) -> Self {
        self.auto_version = on;
        self
    }

    /// Generates a minimal usage string (without validation).
    pub fn usage(&self) -> String {
        let mut parts = vec![Cow::Borrowed("Usage:"), Cow::Owned(self.bin.clone())];
        if self.auto_help
            || self.auto_version
            || self.args.iter().any(|a| !matches!(a.kind, ArgKind::Pos))
        {
            parts.push(Cow::Borrowed("[OPTIONS]"));
        }
        if !self.subcommands.is_empty() {
            parts.push(Cow::Borrowed("<SUBCOMMAND>"));
        }
        for arg in self.args.iter().filter(|a| matches!(a.kind, ArgKind::Pos)) {
            let name = arg.value_name.as_deref().unwrap_or(&arg.name);
            parts.push(Cow::Owned(format!("<{}>", name)));
        }
        parts.into_iter().map(|c| c.into_owned()).collect::<Vec<_>>().join(" ")
    }

    /// Returns a placeholder help message.
    pub fn help(&self) -> String {
        let mut out = String::new();
        out.push_str(&self.usage());
        if let Some(about) = &self.about {
            out.push_str("\n\n");
            out.push_str(about);
        }
        out
    }
}

/// Parsing error placeholder.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParseError {
    /// Parsing is not implemented in the stub.
    Unsupported,
}

impl core::fmt::Display for ParseError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            ParseError::Unsupported => write!(f, "argument parsing is not implemented"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ParseError {}

/// Result of argument parsing.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Matches {
    pub flags: Vec<String>,
    pub options: Vec<(String, String)>,
    pub positionals: Vec<String>,
    pub subcommand: Option<(String, Vec<String>)>,
}

impl Matches {
    /// Returns whether a flag is present.
    pub fn contains(&self, name: &str) -> bool {
        self.flags.iter().any(|f| f == name)
    }

    /// Returns the last value for an option, if any.
    pub fn value(&self, name: &str) -> Option<Cow<'_, str>> {
        self.options
            .iter()
            .rev()
            .find(|(n, _)| n == name)
            .map(|(_, v)| Cow::Borrowed(v.as_str()))
    }

    /// Returns all values for an option.
    pub fn values(&self, name: &str) -> Vec<&str> {
        self.options
            .iter()
            .filter(|(n, _)| n == name)
            .map(|(_, v)| v.as_str())
            .collect()
    }
}

/// Parser stub returning empty matches.
pub struct Parser {
    spec: Spec,
}

impl Parser {
    /// Creates a parser for the given specification.
    pub fn new(spec: Spec) -> Self {
        Self { spec }
    }

    /// Parses arguments from the current process environment (no-op).
    pub fn parse_env(&self) -> Result<Matches, ParseError> {
        let _ = &self.spec;
        Err(ParseError::Unsupported)
    }

    /// Parses the provided argument iterator (no-op).
    pub fn parse<I, S>(&self, _args: I) -> Result<Matches, ParseError>
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        Err(ParseError::Unsupported)
    }

    /// Returns reference to underlying spec.
    pub fn spec(&self) -> &Spec {
        &self.spec
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn spec_usage_stub() {
        let spec = Spec::new("demo").arg(Arg::flag("verbose").short('v')).arg(Arg::pos("input"));
        assert!(spec.usage().contains("demo"));
    }
}
