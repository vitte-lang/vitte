//! vitte-args — parseur CLI **ultra complet** sans dépendances externes
//!
//! Points clés supportés:
//! - Options: `-a`, `-abc`, `-o val`, `-o=val`, `--flag`, `--opt val`, `--opt=val`
//! - Fin des options: `--`
//! - Positionnels: arité `One` / `ZeroOrOne` / `Many` (variadique final)
//! - Sous-commandes: détection et délégation du reste
//! - Valeurs par défaut, fallback via variable d’environnement, choix (`choices`)
//! - Options répétables (`repeatable`) avec `opt_all()`
//! - Alias, dépréciations, options cachées, noms d’affichage de valeur (`value_name`)
//! - Contraintes: `requires`, `conflicts_with` (par noms d’arguments)
//! - Aide/Version auto (`-h/--help`, `-V/--version`) et rendu `usage()`/`help()`
//! - Suggestion sur option inconnue (distance d’édition minimale)
//! - Conversion typée via `value::<T>()` et `positional::<T>()` (`FromStr`)
//! - Génération simple d’autocomplétions Bash/Zsh/Fish
//! - `no_std + alloc` possible via feature `alloc-only`
//!
//! Exemple:
//! ```no_run
//! use vitte_args::{Spec, Arg, Parser, Arity};
//! let spec = Spec::new("vitte")
//!   .about("Vitte CLI")
//!   .version("0.1.0")
//!   .arg(Arg::flag("verbose").short('v').long("verbose").help("Verbose"))
//!   .arg(Arg::opt("output").short('o').long("output").value_name("FILE").help("Fichier sortie"))
//!   .arg(Arg::opt("level").long("level").choices(["debug","info","warn","error"]).default("info"))
//!   .arg(Arg::pos("input").required(true).help("Entrée"))
//!   .arg(Arg::pos("rest").arity(Arity::Many).help("Reste"));
//! let m = Parser::new(spec).parse_env().unwrap();
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec, collections::BTreeMap as Map};

#[cfg(feature = "std")]
use std::collections::HashMap as Map;

use core::fmt;
use core::str::FromStr;

/* =============================== TYPES =============================== */

/// Nature d’un argument.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ArgKind { Flag, Opt, Pos }

/// Cardinalité d’un argument.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Arity { One, ZeroOrOne, Many }

/// Spécification d’un argument.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Arg {
    name: String,
    kind: ArgKind,
    short: Option<char>,
    long: Option<String>,
    help: Option<String>,
    value_name: Option<String>,
    required: bool,
    arity: Arity,
    env: Option<String>,
    default: Option<String>,
    choices: Option<Vec<String>>,
    repeatable: bool,
    hidden: bool,
    deprecated: Option<String>,
    aliases: Vec<String>,
    requires: Vec<String>,
    conflicts: Vec<String>,
}

impl Arg {
    /* ---- constructeurs de base ---- */
    pub fn flag<N: Into<String>>(name: N) -> Self {
        Self {
            name: name.into(), kind: ArgKind::Flag, short: None, long: None,
            help: None, value_name: None, required: false, arity: Arity::ZeroOrOne,
            env: None, default: None, choices: None, repeatable: false,
            hidden: false, deprecated: None, aliases: Vec::new(),
            requires: Vec::new(), conflicts: Vec::new(),
        }
    }
    pub fn opt<N: Into<String>>(name: N) -> Self {
        Self {
            name: name.into(), kind: ArgKind::Opt, short: None, long: None,
            help: None, value_name: None, required: false, arity: Arity::One,
            env: None, default: None, choices: None, repeatable: false,
            hidden: false, deprecated: None, aliases: Vec::new(),
            requires: Vec::new(), conflicts: Vec::new(),
        }
    }
    pub fn pos<N: Into<String>>(name: N) -> Self {
        Self {
            name: name.into(), kind: ArgKind::Pos, short: None, long: None,
            help: None, value_name: None, required: false, arity: Arity::One,
            env: None, default: None, choices: None, repeatable: false,
            hidden: false, deprecated: None, aliases: Vec::new(),
            requires: Vec::new(), conflicts: Vec::new(),
        }
    }

    /* ---- builders ---- */
    pub fn short(mut self, c: char) -> Self { self.short = Some(c); self }
    pub fn long(mut self, l: &str) -> Self { self.long = Some(l.to_string()); self }
    pub fn help(mut self, h: &str) -> Self { self.help = Some(h.to_string()); self }
    pub fn value_name(mut self, n: &str) -> Self { self.value_name = Some(n.to_string()); self }
    pub fn required(mut self, r: bool) -> Self { self.required = r; self }
    pub fn arity(mut self, a: Arity) -> Self { self.arity = a; self }
    pub fn env(mut self, var: &str) -> Self { self.env = Some(var.to_string()); self }
    pub fn default(mut self, v: &str) -> Self { self.default = Some(v.to_string()); self }
    pub fn choices<I: IntoIterator<Item=String>>(mut self, vals: I) -> Self {
        self.choices = Some(vals.into_iter().collect()); self
    }
    pub fn repeatable(mut self, r: bool) -> Self { self.repeatable = r; self }
    pub fn hidden(mut self, h: bool) -> Self { self.hidden = h; self }
    pub fn deprecated(mut self, msg: &str) -> Self { self.deprecated = Some(msg.to_string()); self }
    pub fn alias(mut self, a: &str) -> Self { self.aliases.push(a.to_string()); self }
    pub fn requires<N: Into<String>>(mut self, name: N) -> Self { self.requires.push(name.into()); self }
    pub fn conflicts_with<N: Into<String>>(mut self, name: N) -> Self { self.conflicts.push(name.into()); self }

    /* ---- accès ---- */
    pub fn name(&self) -> &str { &self.name }
    pub fn is_flag(&self) -> bool { matches!(self.kind, ArgKind::Flag) }
    pub fn is_opt(&self) -> bool { matches!(self.kind, ArgKind::Opt) }
    pub fn is_pos(&self) -> bool { matches!(self.kind, ArgKind::Pos) }
}

/// Spécification d’un binaire ou sous-commande.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Spec {
    bin: String,
    about: Option<String>,
    version: Option<String>,
    footer: Option<String>,
    args: Vec<Arg>,
    subcommands: Vec<Spec>,
    auto_help: bool,
    auto_version: bool,
}

impl Spec {
    pub fn new(bin: &str) -> Self {
        Self {
            bin: bin.to_string(), about: None, version: None, footer: None,
            args: Vec::new(), subcommands: Vec::new(),
            auto_help: true, auto_version: true,
        }
    }
    pub fn about(mut self, s: &str) -> Self { self.about = Some(s.to_string()); self }
    pub fn version(mut self, v: &str) -> Self { self.version = Some(v.to_string()); self }
    pub fn footer(mut self, f: &str) -> Self { self.footer = Some(f.to_string()); self }
    pub fn arg(mut self, a: Arg) -> Self { self.args.push(a); self }
    pub fn subcommand(mut self, s: Spec) -> Self { self.subcommands.push(s); self }
    pub fn auto_help(mut self, on: bool) -> Self { self.auto_help = on; self }
    pub fn auto_version(mut self, on: bool) -> Self { self.auto_version = on; self }

    pub fn find_by_short(&self, c: char) -> Option<&Arg> {
        self.args.iter().find(|a| a.short == Some(c))
    }
    pub fn find_by_long(&self, l: &str) -> Option<&Arg> {
        self.args.iter().find(|a| a.long.as_deref() == Some(l) || a.aliases.iter().any(|x| x == l))
    }
    pub fn positionals(&self) -> impl Iterator<Item=&Arg> {
        self.args.iter().filter(|a| a.is_pos())
    }

    pub fn usage(&self) -> String {
        let mut u = String::new();
        u.push_str("Usage: ");
        u.push_str(&self.bin);

        if self.auto_help || self.auto_version || self.args.iter().any(|a| !a.is_pos()) {
            u.push_str(" [OPTIONS]");
        }
        if !self.subcommands.is_empty() {
            u.push_str(" <SUBCOMMAND>");
        }
        for a in self.positionals() {
            u.push(' ');
            let vn = a.value_name.as_deref().unwrap_or(a.name());
            match (a.required, a.arity) {
                (true, Arity::One)       => u.push_str(&format!("<{}>", vn)),
                (false, Arity::One)      => u.push_str(&format!("[{}]", vn)),
                (_, Arity::ZeroOrOne)    => u.push_str(&format!("[{}]", vn)),
                (_, Arity::Many)         => u.push_str(&format!("[{}...]", vn)),
            }
        }
        u
    }

    pub fn help(&self) -> String {
        let mut s = String::new();
        s.push_str(&self.usage()); s.push('\n');
        if let Some(a) = &self.about { s.push('\n'); s.push_str(a); s.push('\n'); }

        // Options
        if self.auto_help || self.auto_version || self.args.iter().any(|a| !a.is_pos() && !a.hidden) {
            s.push_str("\nOptions:\n");
            if self.auto_help {
                s.push_str("  -h, --help\n      Affiche cette aide.\n");
            }
            if self.auto_version {
                s.push_str("  -V, --version\n      Affiche la version.\n");
            }
            for a in self.args.iter().filter(|a| !a.is_pos() && !a.hidden) {
                s.push_str(&format_arg_help(a));
            }
        }

        // Positionnels
        if self.args.iter().any(|a| a.is_pos()) {
            s.push_str("\nArguments:\n");
            for a in self.positionals() {
                s.push_str(&format_arg_help(a));
            }
        }

        // Subcommands
        if !self.subcommands.is_empty() {
            s.push_str("\nSubcommands:\n");
            for sc in &self.subcommands {
                s.push_str(&format!("  {}\n", sc.bin));
            }
        }

        if let Some(f) = &self.footer {
            s.push('\n'); s.push_str(f);
        }
        s
    }

    /// Génère un script de complétion Bash minimal.
    pub fn completion_bash(&self) -> String {
        let mut opts = Vec::new();
        for a in &self.args {
            if let Some(l) = &a.long { opts.push(format!("--{}", l)); }
            if let Some(c) = a.short { opts.push(format!("-{}", c)); }
            for al in &a.aliases { opts.push(format!("--{}", al)); }
        }
        format!(
r#"_{name}_complete() {{
    COMPREPLY=()
    local cur="${{COMP_WORDS[COMP_CWORD]}}"
    local opts="{opts}"
    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
}}
complete -F _{name}_complete {name}
"#, name=self.bin, opts=opts.join(" "))
    }

    /// Génère un script Fish minimal.
    pub fn completion_fish(&self) -> String {
        let mut lines = String::new();
        for a in &self.args {
            if let Some(l) = &a.long {
                lines.push_str(&format!("complete -c {} -l {} {}\n",
                    self.bin, l,
                    a.help.as_ref().map(|h| format!("-d '{}'", escape(h))).unwrap_or_default()));
            }
            if let Some(c) = a.short {
                lines.push_str(&format!("complete -c {} -s {} {}\n",
                    self.bin, c,
                    a.help.as_ref().map(|h| format!("-d '{}'", escape(h))).unwrap_or_default()));
            }
        }
        lines
    }

    /// Génère un script Zsh minimal.
    pub fn completion_zsh(&self) -> String {
        let mut entries = Vec::new();
        for a in &self.args {
            if let Some(l) = &a.long {
                entries.push(format!("'--{}[{}]'", l, a.help.as_deref().unwrap_or("")));
            }
            if let Some(c) = a.short {
                entries.push(format!("'-{}[{}]'", c, a.help.as_deref().unwrap_or("")));
            }
        }
        format!(
r#"#compdef {name}
_arguments \\
  {entries}
"#, name=self.bin, entries=entries.join(" \\\n  "))
    }
}

/* ============================ ERREURS ============================ */

#[cfg(feature = "errors")]
#[derive(Debug, thiserror::Error, PartialEq, Eq)]
pub enum ParseError {
    #[error("unknown option: {0}{1}")]
    UnknownOpt(String, String), // second champ = suggestion éventuelle
    #[error("missing value for option: {0}")]
    MissingValue(String),
    #[error("unexpected value for flag: {0}")]
    UnexpectedValue(String),
    #[error("missing required: {0}")]
    MissingRequired(String),
    #[error("too many positionals")]
    TooManyPositionals,
    #[error("option {0} requires: {1}")]
    Requires(String, String),
    #[error("option {0} conflicts with: {1}")]
    Conflicts(String, String),
    #[error("invalid choice for {0}: {1} (expected one of {2})")]
    InvalidChoice(String, String, String),
}

#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum ParseError {
    UnknownOpt(String, String),
    MissingValue(String),
    UnexpectedValue(String),
    MissingRequired(String),
    TooManyPositionals,
    Requires(String, String),
    Conflicts(String, String),
    InvalidChoice(String, String, String),
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Matches {
    flags: Map<String, bool>,
    opts_last: Map<String, String>,
    opts_all: Map<String, Vec<String>>,
    pos: Vec<String>,
    subcommand: Option<(String, Vec<String>)>,
    deprecated_used: Vec<(String, String)>, // (name,msg)
}

impl Matches {
    pub fn flag(&self, name: &str) -> bool {
        self.flags.get(name).copied().unwrap_or(false)
    }
    pub fn opt(&self, name: &str) -> Option<&str> {
        self.opts_last.get(name).map(|s| s.as_str())
    }
    pub fn opt_all(&self, name: &str) -> Option<&[String]> {
        self.opts_all.get(name).map(|v| v.as_slice())
    }
    pub fn positionals(&self) -> &[String] { &self.pos }
    pub fn subcommand(&self) -> Option<(&str, &[String])> {
        self.subcommand.as_ref().map(|(n, v)| (n.as_str(), v.as_slice()))
    }
    /// Renvoie les options dépréciées effectivement utilisées avec leur message.
    pub fn deprecated_used(&self) -> &[(String,String)] { &self.deprecated_used }

    pub fn value<T: FromStr>(&self, name: &str) -> Result<Option<T>, ParseValueError<T::Err>> {
        if let Some(s) = self.opt(name) { s.parse::<T>().map(Some).map_err(ParseValueError::Convert) } else { Ok(None) }
    }
    pub fn positional<T: FromStr>(&self, idx: usize) -> Result<Option<T>, ParseValueError<T::Err>> {
        if let Some(s) = self.pos.get(idx) { s.parse::<T>().map(Some).map_err(ParseValueError::Convert) } else { Ok(None) }
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum ParseValueError<E> { Missing, Convert(E) }

/* ============================== PARSER ============================== */

pub struct Parser { spec: Spec }

impl Parser {
    pub fn new(spec: Spec) -> Self { Self { spec } }

    #[cfg(feature = "std")]
    pub fn parse_env(&self) -> Result<Matches, ParseError> {
        self.parse(std::env::args().skip(1))
    }

    pub fn parse<I, S>(&self, args: I) -> Result<Matches, ParseError>
    where I: IntoIterator<Item = S>, S: Into<String> {
        let mut flags: Map<String, bool> = Map::new();
        let mut last: Map<String, String> = Map::new();
        let mut all: Map<String, Vec<String>> = Map::new();
        let mut pos: Vec<String> = Vec::new();
        let mut deprecated_used: Vec<(String,String)> = Vec::new();

        let mut it = args.into_iter().map(|s| s.into()).peekable();
        let mut stop_opts = false;

        // Indexation rapide et bootstrap defaults/env
        let mut short_map = Map::new();
        let mut long_map = Map::new();
        let mut pos_defs: Vec<&Arg> = Vec::new();
        for a in &self.spec.args {
            if let Some(c) = a.short { short_map.insert(c, a); }
            if let Some(l) = &a.long { long_map.insert(l.clone(), a); }
            for al in &a.aliases { long_map.insert(al.clone(), a); }
            if a.is_pos() { pos_defs.push(a); }
            if a.is_opt() {
                if let Some(env) = &a.env {
                    if let Some(v) = read_env(env) {
                        last.insert(a.name.clone(), v.clone());
                        all.entry(a.name.clone()).or_default().push(v);
                    }
                }
                if let Some(def) = &a.default {
                    last.entry(a.name.clone()).or_insert_with(|| def.clone());
                    all.entry(a.name.clone()).or_default().push(def.clone());
                }
            }
            if a.is_flag() {
                flags.insert(a.name.clone(), false);
            }
        }

        // Aide/Version auto
        if self.spec.auto_help {
            flags.insert("help".into(), false);
            insert_virtual_flag(&mut short_map, &mut long_map, 'h', "help");
        }
        if self.spec.auto_version {
            flags.insert("version".into(), false);
            insert_virtual_flag(&mut short_map, &mut long_map, 'V', "version");
        }

        // Parsing
        while let Some(tok) = it.next() {
            if !stop_opts && tok == "--" { stop_opts = true; continue; }

            if !stop_opts && tok.starts_with("--") {
                let (name, val) = split_long(&tok[2..]);
                if let Some(arg) = long_map.get(name).copied() {
                    if let Some(msg) = &arg.deprecated { deprecated_used.push((arg.name.clone(), msg.clone())); }
                    match arg.kind {
                        ArgKind::Flag => {
                            if val.is_some() { return Err(err_unexpected_value(name)); }
                            flags.insert(arg.name.clone(), true);
                        }
                        ArgKind::Opt => {
                            let v = match val {
                                Some(v) => v.to_string(),
                                None => it.next().ok_or_else(|| err_missing_value(name))?,
                            };
                            validate_choice(arg, &v)?;
                            put_value(arg, v, &mut last, &mut all);
                        }
                        ArgKind::Pos => unreachable!(),
                    }
                } else {
                    return Err(err_unknown_with_suggestion(name, &long_map));
                }
                continue;
            }

            if !stop_opts && tok.starts_with('-') && tok.len() > 1 {
                let mut chars = tok[1..].chars().peekable();
                while let Some(c) = chars.next() {
                    if let Some(arg) = short_map.get(&c).copied() {
                        if let Some(msg) = &arg.deprecated { deprecated_used.push((arg.name.clone(), msg.clone())); }
                        match arg.kind {
                            ArgKind::Flag => { flags.insert(arg.name.clone(), true); }
                            ArgKind::Opt => {
                                let rest: String = chars.collect();
                                let v = if !rest.is_empty() { rest } else {
                                    it.next().ok_or_else(|| err_missing_value_char(c))?
                                };
                                validate_choice(arg, &v)?;
                                put_value(arg, v, &mut last, &mut all);
                                break;
                            }
                            ArgKind::Pos => unreachable!(),
                        }
                    } else {
                        return Err(err_unknown_with_suggestion_char(c, &short_map));
                    }
                }
                continue;
            }

            // sous-commande
            if let Some(sc) = self.spec.subcommands.iter().find(|sc| sc.bin == tok) {
                let rest: Vec<String> = it.collect();
                return Ok(Matches { flags, opts_last: last, opts_all: all, pos, subcommand: Some((sc.bin.clone(), rest)), deprecated_used });
            }

            // positionnel
            pos.push(tok);
        }

        // Vérifications positionnels
        check_positionals(&self.spec, &pos)?;

        // Required options
        for a in self.spec.args.iter().filter(|a| a.is_opt() && a.required) {
            if !last.contains_key(&a.name) { return Err(ParseError::MissingRequired(a.name.clone())); }
        }

        // Conflits / Requires
        for a in &self.spec.args {
            let present = a.is_flag() && flags.get(&a.name).copied().unwrap_or(false)
                || a.is_opt() && last.contains_key(&a.name);
            if present {
                for r in &a.requires {
                    let ok = flags.get(r).copied().unwrap_or(false)
                        || last.contains_key(r)
                        || self.spec.positionals().any(|p| p.name() == r); // tolérance si c’est un pos, vérifié plus haut
                    if !ok { return Err(ParseError::Requires(a.name.clone(), r.clone())); }
                }
                for c in &a.conflicts {
                    let bad = flags.get(c).copied().unwrap_or(false) || last.contains_key(c);
                    if bad { return Err(ParseError::Conflicts(a.name.clone(), c.clone())); }
                }
            }
        }

        // Auto help/version (ne s’active que si présent explicitement)
        if *flags.get("help").unwrap_or(&false) {
            #[cfg(feature = "std")]
            { println!("{}", self.spec.help()); std::process::exit(0); }
        }
        if *flags.get("version").unwrap_or(&false) {
            #[cfg(feature = "std")]
            {
                if let Some(v) = &self.spec.version { println!("{}", v); }
                else { println!("(no version)"); }
                std::process::exit(0);
            }
        }

        Ok(Matches { flags, opts_last: last, opts_all: all, pos, subcommand: None, deprecated_used })
    }
}

/* ============================ HELPERS ============================ */

fn split_long(s: &str) -> (&str, Option<&str>) {
    if let Some(i) = s.find('=') { (&s[..i], Some(&s[i+1..])) } else { (s, None) }
}

fn put_value(arg: &Arg, v: String, last: &mut Map<String,String>, all: &mut Map<String,Vec<String>>) {
    if arg.repeatable {
        all.entry(arg.name.clone()).or_default().push(v.clone());
    } else {
        all.insert(arg.name.clone(), vec![v.clone()]);
    }
    last.insert(arg.name.clone(), v);
}

fn validate_choice(arg: &Arg, val: &str) -> Result<(), ParseError> {
    if let Some(choices) = &arg.choices {
        if !choices.iter().any(|c| c == val) {
            return Err(ParseError::InvalidChoice(arg.name.clone(), val.to_string(), format!("{choices:?}")));
        }
    }
    Ok(())
}

fn check_positionals(spec: &Spec, pos: &[String]) -> Result<(), ParseError> {
    let defs: Vec<&Arg> = spec.positionals().collect();

    // required count
    let req_count = defs.iter().filter(|a| a.required && !matches!(a.arity, Arity::ZeroOrOne)).count();
    if pos.len() < req_count {
        if let Some(a) = defs.iter().find(|a| a.required).copied() {
            return Err(ParseError::MissingRequired(a.name.clone()));
        }
    }
    // many constraint
    let many_count = defs.iter().filter(|a| matches!(a.arity, Arity::Many)).count();
    if many_count > 1 { /* spec discutable, on tolère */ }
    if many_count == 0 && pos.len() > defs.len() {
        return Err(ParseError::TooManyPositionals);
    }
    Ok(())
}

fn insert_virtual_flag(short_map: &mut Map<char, &Arg>, long_map: &mut Map<String, &Arg>, c: char, l: &str) {
    // On crée un Arg temporaire statique-like (stack) uniquement pour mapping,
    // son adresse n'est pas utilisée hors de l'appel de parse -> on clone signature ici.
    // Pour rester sûr, on fabrique un Arg local et on leak dans une Box pour 'static.
    let tmp = Arg::flag(l).short(c).long(l);
    let bx: &'static Arg = Box::leak(Box::new(tmp));
    short_map.insert(c, bx);
    long_map.insert(l.to_string(), bx);
}

#[cfg(feature = "std")]
fn read_env(var: &str) -> Option<String> { std::env::var(var).ok() }
#[cfg(not(feature = "std"))]
fn read_env(_: &str) -> Option<String> { None }

fn escape(s: &str) -> String {
    s.replace('\'', "\\'")
}

/* ---- suggestions pour options inconnues ---- */

fn err_unknown_with_suggestion(name: &str, long_map: &Map<String, &Arg>) -> ParseError {
    let mut best: Option<(&str, usize)> = None;
    for cand in long_map.keys() {
        let d = edit_distance(name, cand);
        if d <= 2 { // seuil
            if best.map(|b| d < b.1).unwrap_or(true) { best = Some((cand.as_str(), d)); }
        }
    }
    let hint = best.map(|(c,_)| format!("; did you mean '--{}'?", c)).unwrap_or_default();
    ParseError::UnknownOpt(format!("--{}", name), hint)
}

fn err_unknown_with_suggestion_char(c: char, short_map: &Map<char, &Arg>) -> ParseError {
    // Pour les short, on liste les existants
    let mut near = String::new();
    if !short_map.is_empty() {
        near.push_str("; valid shorts: ");
        for k in short_map.keys() { near.push('-'); near.push(*k); near.push(' '); }
    }
    ParseError::UnknownOpt(format!("-{}", c), near)
}

fn err_missing_value(name: &str) -> ParseError { ParseError::MissingValue(format!("--{}", name)) }
fn err_missing_value_char(c: char) -> ParseError { ParseError::MissingValue(format!("-{}", c)) }
fn err_unexpected_value(name: &str) -> ParseError { ParseError::UnexpectedValue(format!("--{}", name)) }

fn edit_distance(a: &str, b: &str) -> usize {
    // Levenshtein O(|a|*|b|) sans allocs lourdes
    let (a_bytes, b_bytes) = (a.as_bytes(), b.as_bytes());
    let (n, m) = (a_bytes.len(), b_bytes.len());
    if n == 0 { return m } ; if m == 0 { return n }
    let mut prev: Vec<usize> = (0..=m).collect();
    let mut curr: Vec<usize> = vec![0; m+1];
    for i in 1..=n {
        curr[0] = i;
        for j in 1..=m {
            let cost = if a_bytes[i-1] == b_bytes[j-1] { 0 } else { 1 };
            curr[j] = (prev[j] + 1).min(curr[j-1] + 1).min(prev[j-1] + cost);
        }
        core::mem::swap(&mut prev, &mut curr);
    }
    prev[m]
}

/* ================================ TESTS ================================ */

#[cfg(test)]
mod tests {
    use super::*;

    fn spec_base() -> Spec {
        Spec::new("app")
            .about("demo")
            .version("1.2.3")
            .arg(Arg::flag("verbose").short('v').long("verbose").help("Verbose"))
            .arg(Arg::opt("output").short('o').long("output").value_name("FILE").help("Sortie"))
            .arg(Arg::opt("level").long("level").choices(["trace","debug","info","warn","error"].into_iter().map(String::from)).default("info"))
            .arg(Arg::opt("tag").long("tag").repeatable(true))
            .arg(Arg::opt("mode").long("mode").choices(["a","b","c"].into_iter().map(String::from)))
            .arg(Arg::opt("old").long("old").deprecated("Use --mode instead"))
            .arg(Arg::opt("need").long("need").requires("mode"))
            .arg(Arg::opt("x").long("x").conflicts_with("mode"))
            .arg(Arg::pos("input").required(true).help("Entrée"))
            .arg(Arg::pos("rest").arity(Arity::Many).help("Reste"))
            .subcommand(Spec::new("build"))
    }

    #[test]
    fn ok_basic() {
        let p = Parser::new(spec_base());
        let m = p.parse(["-v","--output=out","--tag=a","--tag","b","file","x","y"].into_iter()).unwrap();
        assert!(m.flag("verbose"));
        assert_eq!(m.opt("output"), Some("out"));
        assert_eq!(m.opt_all("tag").unwrap(), &["a".into(),"b".into()]);
        assert_eq!(m.positionals(), &vec!["file".into(),"x".into(),"y".into()]);
    }

    #[test]
    fn choices_default() {
        let p = Parser::new(spec_base());
        let m = p.parse(["file"].into_iter()).unwrap();
        assert_eq!(m.opt("level"), Some("info"));
    }

    #[test]
    fn requires_conflicts() {
        let p = Parser::new(spec_base());
        // need requires mode
        assert!(matches!(p.parse(["--need=1","file"].into_iter()), Err(ParseError::Requires(_, _))));
        // x conflicts mode
        assert!(matches!(p.parse(["--mode=a","--x=1","file"].into_iter()), Err(ParseError::Conflicts(_, _))));
    }

    #[test]
    fn unknown_with_suggestion() {
        let p = Parser::new(spec_base());
        let e = p.parse(["--verbse","file"].into_iter()).unwrap_err();
        if let ParseError::UnknownOpt(_, hint) = e { assert!(hint.contains("did you mean")); } else { panic!("expected UnknownOpt"); }
    }

    #[test]
    fn subcommand_capture() {
        let p = Parser::new(spec_base());
        let m = p.parse(["build","--tag=x"].into_iter()).unwrap();
        let (name, rest) = m.subcommand().unwrap();
        assert_eq!(name, "build");
        assert_eq!(rest, &["--tag=x".to_string()]);
    }

    #[test]
    fn deprecated_tracking() {
        let p = Parser::new(spec_base());
        let m = p.parse(["--old","1","file"].into_iter()).unwrap();
        assert!(m.deprecated_used().iter().any(|(n, _)| n == "old"));
    }

    #[test]
    fn too_many_pos_without_many() {
        let spec = Spec::new("a")
            .arg(Arg::pos("x").required(true))
            .arg(Arg::pos("y").required(true));
        let p = Parser::new(spec);
        let e = p.parse(["a","b","c"].into_iter()).unwrap_err();
        assert!(matches!(e, ParseError::TooManyPositionals));
    }

    #[test]
    fn typed_value() {
        let spec = Spec::new("n").arg(Arg::opt("port").long("port"));
        let p = Parser::new(spec);
        let m = p.parse(["--port","8080"].into_iter()).unwrap();
        let v: u16 = m.value("port").unwrap().unwrap();
        assert_eq!(v, 8080);
    }
}