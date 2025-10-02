//! vitte-completion — génération d’autocomplétions **ultra complète** (Bash, Zsh, Fish)
//!
//! Capacités:
//! - Options courtes/longues, alias, répétables
//! - Sous-commandes récursives
//! - Valeurs avec *hints* (fichier, dossier, commande, choix…)
//! - Intégration directe depuis `vitte-args::Spec` (feature `args-spec`)
//! - Générateurs dynamiques via un `Completer` utilisateur
//! - Scripts prêts à être écrits dans les emplacements standards
//!
//! Exemple rapide (avec `args-spec`):
//! ```no_run
//! use vitte_args::{Spec, Arg};
//! use vitte_completion::{CompletionGenerator, Shell, ValueHint};
//! let spec = Spec::new("vitte")
//!   .about("Vitte CLI")
//!   .arg(Arg::flag("verbose").short('v').long("verbose"))
//!   .arg(Arg::opt("output").long("output").value_name("FILE"));
//! let gen = CompletionGenerator::from_args_spec(&spec)
//!   .hint_long("output", ValueHint::File);
//! println!("{}", gen.generate(Shell::Zsh).unwrap());
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`).");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec, collections::BTreeMap as Map, format};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec, collections::HashMap as Map, fmt::Write as _};

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "args-spec")]
use vitte_args::{Spec as ArgsSpec, Arg as ArgsArg, ArgKind as ArgsArgKind};

/// Shell cible.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Shell { Bash, Zsh, Fish }

/// Erreurs.
#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum CompletionError {
    #[error("unsupported shell")]
    Unsupported,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum CompletionError { Unsupported }

#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, CompletionError>;
#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/// Indice de complétion pour valeurs.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ValueHint {
    Any,
    File,
    Directory,
    Command,
    Choice(Vec<String>),
}

/// Modèle d’option exportée pour completion.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OptSpec {
    pub name: String,               // nom logique
    pub short: Option<char>,
    pub long: Option<String>,
    pub aliases: Vec<String>,
    pub help: Option<String>,
    pub takes_value: bool,
    pub repeatable: bool,
    pub value_name: Option<String>,
    pub hint: ValueHint,
    pub hidden: bool,
}

/// Modèle d’argument positionnel.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PosSpec {
    pub name: String,
    pub help: Option<String>,
    pub variadic: bool,
    pub hint: ValueHint,
}

/// Spéc d’une commande (racine ou sous-commande).
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CmdSpec {
    pub bin: String,
    pub about: Option<String>,
    pub options: Vec<OptSpec>,
    pub positionals: Vec<PosSpec>,
    pub subcommands: Vec<CmdSpec>,
}

/* ======================== COMPLETER DYNAMIQUE ======================== */

/// Fournit des valeurs possibles dynamiquement selon le *cursor context*.
/// `words` = tokens courants, `current` = préfixe à compléter.
pub trait Completer: Send + Sync {
    fn candidates(&self, words: &[&str], current: &str) -> Vec<String>;
}

impl<F> Completer for F
where F: Fn(&[&str], &str) -> Vec<String> + Send + Sync {
    fn candidates(&self, words: &[&str], current: &str) -> Vec<String> { (self)(words, current) }
}

/* ======================== GENERATOR PRINCIPAL ======================== */

pub struct CompletionGenerator {
    spec: CmdSpec,
    long_hints: Map<String, ValueHint>,   // override par nom long
    short_hints: Map<char, ValueHint>,    // override par short
    pos_hints: Map<String, ValueHint>,    // override par pos
    completer: Option<Box<dyn Completer>>,
}

impl CompletionGenerator {
    /// Construire depuis une `CmdSpec` manuelle.
    pub fn new(spec: CmdSpec) -> Self {
        Self {
            spec,
            long_hints: Map::new(),
            short_hints: Map::new(),
            pos_hints: Map::new(),
            completer: None,
        }
    }

    /// Source `vitte-args::Spec` → `CompletionGenerator`.
    #[cfg(feature = "args-spec")]
    pub fn from_args_spec(spec: &ArgsSpec) -> Self {
        Self::new(convert_from_args(spec))
    }

    /// Ajoute un *hint* pour `--long`.
    pub fn hint_long(mut self, long: &str, hint: ValueHint) -> Self {
        self.long_hints.insert(long.to_string(), hint);
        self
    }

    /// Ajoute un *hint* pour `-x`.
    pub fn hint_short(mut self, short: char, hint: ValueHint) -> Self {
        self.short_hints.insert(short, hint);
        self
    }

    /// Ajoute un *hint* pour positionnel.
    pub fn hint_pos(mut self, name: &str, hint: ValueHint) -> Self {
        self.pos_hints.insert(name.to_string(), hint);
        self
    }

    /// Active un compléteur dynamique.
    pub fn with_completer(mut self, c: Box<dyn Completer>) -> Self {
        self.completer = Some(c);
        self
    }

    /// Génère le script pour `sh`.
    pub fn generate(&self, sh: Shell) -> Result<String> {
        match sh {
            Shell::Bash => Ok(self.gen_bash()),
            Shell::Zsh  => Ok(self.gen_zsh()),
            Shell::Fish => Ok(self.gen_fish()),
        }
    }

    /* --------------------------- BASH --------------------------- */

    fn gen_bash(&self) -> String {
        let mut s = String::new();
        let name = &self.spec.bin;
        // Header
        pushln(&mut s, &format!("_{name}_complete() {{"));
        pushln(&mut s, "    COMPREPLY=()");
        pushln(&mut s, "    local cur prev words cword");
        pushln(&mut s, "    _init_completion -n : || return");
        // Dispatcher récursif
        self.bash_cmd(&mut s, &self.spec, "words", 0);
        pushln(&mut s, "}");
        pushln(&mut s, &format!("complete -F _{name}_complete {name}"));
        s
    }

    // Génère un bloc case pour une commande donnée.
    fn bash_cmd(&self, out: &mut String, cmd: &CmdSpec, words_var: &str, depth: usize) {
        // Options et sous-commandes
        let opts = self.collect_bash_opts(cmd);
        let subs: Vec<&str> = cmd.subcommands.iter().map(|sc| sc.bin.as_str()).collect();

        pushln(out, &format!("    # {}", cmd.bin));
        pushln(out, &format!("    case ${} in", format!("{}_{}", words_var, depth).to_uppercase()));

        // Par défaut: définir variables d’index
        // On prépare `idx=<depth>` et extrait le token courant.
        pushln(out, &format!("    esac"));

        // Corps principal: on reconstruit une machine simple
        // 1) Si le mot précédent est une option à valeur → compléter selon hint
        // 2) Sinon si on tape une option → proposer options
        // 3) Sinon si on tape première positionnelle → proposer subcommands sinon fichiers/dirs selon hint du pos
        // Implémentation pragmatique via _get_comp_words_by_ref
        out.clear();
        // Regénération plus directive:
        let mut body = String::new();
        pushln(&mut body, "    local i");
        pushln(&mut body, &format!("    local -a __opts __subs __cands"));
        pushln(&mut body, &format!("    __opts=({})", opts.join(" ")));
        if subs.is_empty() { pushln(&mut body, "    __subs=()"); }
        else { pushln(&mut body, &format!("    __subs=({})", subs.join(" "))); }
        pushln(&mut body, "    # Détection option précédente nécessitant une valeur");
        // Table des longs→hint
        let mut kv = Vec::new();
        for o in &cmd.options {
            if o.takes_value {
                let hint = self.resolve_opt_hint(o);
                kv.push((o.long.clone().unwrap_or_default(), hint.clone()));
                if let Some(c) = o.short {
                    kv.push((format!("-{}", c), hint));
                }
            }
        }
        pushln(&mut body, "    local need_val=''");
        pushln(&mut body, "    if [[ ${#COMP_WORDS[@]} -ge 2 ]]; then");
        pushln(&mut body, "      local prev=${COMP_WORDS[COMP_CWORD-1]}");
        // Vérifie toutes les clés
        for (k, _) in &kv {
            if k.is_empty() { continue; }
            pushln(&mut body, &format!("      if [[ \"$prev\" == \"--{k}\" || \"$prev\" == \"{k}\" ]]; then need_val=\"{k}\"; fi"));
        }
        pushln(&mut body, "    fi");
        // Si need_val, on complète valeur
        pushln(&mut body, "    if [[ -n \"$need_val\" ]]; then");
        pushln(&mut body, "      case \"$need_val\" in");
        for (k, hint) in &kv {
            if k.is_empty() { continue; }
            pushln(&mut body, &format!("        \"--{k}\"|\"{k}\")"));
            self.bash_value_hint(&mut body, hint);
            pushln(&mut body, "        ;;");
        }
        pushln(&mut body, "      esac");
        pushln(&mut body, "      return 0");
        pushln(&mut body, "    fi");
        // Sinon: proposer options + sous-commandes si premier positionnel
        pushln(&mut body, "    local cur=${COMP_WORDS[COMP_CWORD]}");
        pushln(&mut body, "    if [[ \"$cur\" == -* ]]; then");
        pushln(&mut body, "      COMPREPLY=( $(compgen -W \"${__opts[*]}\" -- \"$cur\") )");
        pushln(&mut body, "      return 0");
        pushln(&mut body, "    fi");
        if !subs.is_empty() {
            pushln(&mut body, "    # Si première positionnelle: proposer sous-commandes");
            pushln(&mut body, "    local first_pos=1");
            // heuristique: si le premier non-option est déjà consommé et correspond à une sous-commande, on délègue
            pushln(&mut body, "    for ((i=1;i<COMP_CWORD;i++)); do");
            pushln(&mut body, "      local w=${COMP_WORDS[i]}");
            pushln(&mut body, "      if [[ \"$w\" != -* ]]; then first_pos=0; fi");
            pushln(&mut body, "    done");
            pushln(&mut body, "    if [[ $first_pos -eq 1 ]]; then");
            pushln(&mut body, "      COMPREPLY=( $(compgen -W \"${__subs[*]}\" -- \"$cur\") )");
            pushln(&mut body, "      return 0");
            pushln(&mut body, "    fi");
        }
        // Sinon positionnels restants → appliquer hint du premier pos non consommé
        if let Some(pos) = cmd.positionals.first() {
            pushln(&mut body, "    # Positionnels restants → hint du premier défini");
            self.bash_value_hint(&mut body, &self.resolve_pos_hint(pos));
        } else {
            pushln(&mut body, "    COMPREPLY=()");
        }

        body
            // inject
            .split('\n')
            .for_each(|ln| { pushln(out, ln); });

        // Sous-commandes récursives: on ajoute un *case* sur le premier non-option
        if !cmd.subcommands.is_empty() {
            pushln(out, "    # Délégation vers sous-commandes");
            pushln(out, "    local sub=''");
            pushln(out, "    for ((i=1;i<COMP_CWORD;i++)); do");
            pushln(out, "      if [[ ${COMP_WORDS[i]} != -* ]]; then sub=${COMP_WORDS[i]}; break; fi");
            pushln(out, "    done");
            pushln(out, "    case \"$sub\" in");
            for sc in &cmd.subcommands {
                pushln(out, &format!("      {})", sc.bin));
                // Pour déléguer, on régénère la logique locale dans un sous-scope.
                // Simplicité: regénérer un bloc identique mais avec ce CmdSpec.
                let sub_block = self.sub_bash_block(sc);
                sub_block.lines().for_each(|ln| pushln(out, &format!("        {}", ln)));
                pushln(out, "        ;;");
            }
            pushln(out, "    esac");
        }
    }

    fn sub_bash_block(&self, cmd: &CmdSpec) -> String {
        // Reutilise la même logique que bash_cmd mais inline.
        let mut s = String::new();
        let opts = self.collect_bash_opts(cmd);
        let subs: Vec<&str> = cmd.subcommands.iter().map(|sc| sc.bin.as_str()).collect();
        pushln(&mut s, &format!("local -a __opts __subs __cands; __opts=({})", opts.join(" ")));
        if subs.is_empty() { pushln(&mut s, "__subs=()"); }
        else { pushln(&mut s, &format!("__subs=({})", subs.join(" "))); }
        // need_val
        let mut kv = Vec::new();
        for o in &cmd.options {
            if o.takes_value {
                let hint = self.resolve_opt_hint(o);
                kv.push((o.long.clone().unwrap_or_default(), hint.clone()));
                if let Some(c) = o.short { kv.push((format!("-{}", c), hint)); }
            }
        }
        pushln(&mut s, "local need_val=''");
        pushln(&mut s, "if [[ ${#COMP_WORDS[@]} -ge 2 ]]; then");
        pushln(&mut s, "  local prev=${COMP_WORDS[COMP_CWORD-1]}");
        for (k, _) in &kv {
            if k.is_empty() { continue; }
            pushln(&mut s, &format!("  if [[ \"$prev\" == \"--{k}\" || \"$prev\" == \"{k}\" ]]; then need_val=\"{k}\"; fi"));
        }
        pushln(&mut s, "fi");
        pushln(&mut s, "if [[ -n \"$need_val\" ]]; then");
        pushln(&mut s, "  case \"$need_val\" in");
        for (k, hint) in &kv {
            if k.is_empty() { continue; }
            pushln(&mut s, &format!("    \"--{k}\"|\"{k}\")"));
            self.bash_value_hint(&mut s, hint);
            pushln(&mut s, "    ;;");
        }
        pushln(&mut s, "  esac; return 0; fi");
        pushln(&mut s, "local cur=${COMP_WORDS[COMP_CWORD]}");
        pushln(&mut s, "if [[ \"$cur\" == -* ]]; then COMPREPLY=( $(compgen -W \"${__opts[*]}\" -- \"$cur\") ); return 0; fi");
        if !subs.is_empty() {
            pushln(&mut s, "local first_pos=1; for ((i=1;i<COMP_CWORD;i++)); do local w=${COMP_WORDS[i]}; if [[ \"$w\" != -* ]]; then first_pos=0; fi; done");
            pushln(&mut s, "if [[ $first_pos -eq 1 ]]; then COMPREPLY=( $(compgen -W \"${__subs[*]}\" -- \"$cur\") ); return 0; fi");
        }
        if let Some(pos) = cmd.positionals.first() {
            self.bash_value_hint(&mut s, &self.resolve_pos_hint(pos));
        } else {
            pushln(&mut s, "COMPREPLY=()");
        }
        s
    }

    fn bash_value_hint(&self, out: &mut String, hint: &ValueHint) {
        match hint {
            ValueHint::Any => pushln(out, "      COMPREPLY=();"),
            ValueHint::File => pushln(out, "      COMPREPLY=( $(compgen -f -- \"$cur\") );"),
            ValueHint::Directory => pushln(out, "      COMPREPLY=( $(compgen -d -- \"$cur\") );"),
            ValueHint::Command => pushln(out, "      COMPREPLY=( $(compgen -c -- \"$cur\") );"),
            ValueHint::Choice(list) => {
                let joined = shell_join(list);
                pushln(out, &format!("      COMPREPLY=( $(compgen -W \"{}\" -- \"$cur\") );", joined));
            }
        }
    }

    fn collect_bash_opts(&self, cmd: &CmdSpec) -> Vec<String> {
        let mut v = Vec::new();
        for o in &cmd.options {
            if o.hidden { continue; }
            if let Some(l) = &o.long { v.push(format!("--{}", l)); }
            if let Some(c) = o.short { v.push(format!("-{}", c)); }
            for al in &o.aliases { v.push(format!("--{}", al)); }
        }
        v
    }

    /* ---------------------------- ZSH ---------------------------- */

    fn gen_zsh(&self) -> String {
        let mut s = String::new();
        let name = &self.spec.bin;
        pushln(&mut s, &format!("#compdef {}", name));
        self.zsh_cmd(&mut s, &self.spec, name, 0);
        s
    }

    fn zsh_cmd(&self, out: &mut String, cmd: &CmdSpec, name: &str, depth: usize) {
        pushln(out, "_arguments -s \\");
        // Options
        for o in &cmd.options {
            if o.hidden { continue; }
            let mut ent = String::new();
            if let Some(c) = o.short {
                write!(&mut ent, "'-{}[{}]'", c, o.help.as_deref().unwrap_or("")).ok();
                if o.takes_value {
                    write!(&mut ent, ":{}:", o.value_name.as_deref().unwrap_or("value")).ok();
                    ent.push_str(&self.zsh_hint_suffix(&self.resolve_opt_hint(o)));
                }
                pushln(out, &format!("  {} \\", ent));
                ent.clear();
            }
            if let Some(l) = &o.long {
                write!(&mut ent, "'--{}[{}]'", l, o.help.as_deref().unwrap_or("")).ok();
                if o.takes_value {
                    write!(&mut ent, ":{}:", o.value_name.as_deref().unwrap_or("value")).ok();
                    ent.push_str(&self.zsh_hint_suffix(&self.resolve_opt_hint(o)));
                }
                pushln(out, &format!("  {} \\", ent));
            }
            for al in &o.aliases {
                let mut a = String::new();
                write!(&mut a, "'--{}[alias]' ", al).ok();
                pushln(out, &format!("  {}\\", a.trim_end()));
            }
        }
        // Positionnels
        for p in &cmd.positionals {
            let mut ent = String::new();
            write!(&mut ent, "'{}:", p.name).ok();
            ent.push_str(&self.zsh_hint_suffix(&self.resolve_pos_hint(p)));
            ent.push('\'');
            pushln(out, &format!("  {} \\", ent));
        }
        // Sous-commandes
        if !cmd.subcommands.is_empty() {
            let subs: Vec<String> = cmd.subcommands.iter().map(|sc| sc.bin.clone()).collect();
            pushln(out, &format!("  ':subcmd:({})' \\", subs.join(" ")));
            pushln(out, "  '*::args:->args'");
            // Dispatcher
            pushln(out, &format!("case $state in"));
            pushln(out, "  args)");
            pushln(out, "    case $words[1] in");
            for sc in &cmd.subcommands {
                pushln(out, &format!("      {})", sc.bin));
                // Appel récursif: on définit une nouvelle *function* temporaire
                let child = self.zsh_sub_fn(sc);
                child.lines().for_each(|ln| pushln(out, &format!("        {}", ln)));
                pushln(out, "        ;;");
            }
            pushln(out, "    esac");
            pushln(out, "  ;;");
            pushln(out, "esac");
        }
        let _ = name; let _ = depth;
    }

    fn zsh_sub_fn(&self, sc: &CmdSpec) -> String {
        // On ré-énumère les options/args comme un nouveau _arguments
        let mut s = String::new();
        pushln(&mut s, "_arguments -s \\");
        for o in &sc.options {
            if o.hidden { continue; }
            let mut ent = String::new();
            if let Some(c) = o.short {
                write!(&mut ent, "'-{}[{}]'", c, o.help.as_deref().unwrap_or("")).ok();
                if o.takes_value {
                    write!(&mut ent, ":{}:", o.value_name.as_deref().unwrap_or("value")).ok();
                    ent.push_str(&self.zsh_hint_suffix(&self.resolve_opt_hint(o)));
                }
                pushln(&mut s, &format!("  {} \\", ent));
                ent.clear();
            }
            if let Some(l) = &o.long {
                write!(&mut ent, "'--{}[{}]'", l, o.help.as_deref().unwrap_or("")).ok();
                if o.takes_value {
                    write!(&mut ent, ":{}:", o.value_name.as_deref().unwrap_or("value")).ok();
                    ent.push_str(&self.zsh_hint_suffix(&self.resolve_opt_hint(o)));
                }
                pushln(&mut s, &format!("  {} \\", ent));
            }
        }
        for p in &sc.positionals {
            let mut ent = String::new();
            write!(&mut ent, "'{}:", p.name).ok();
            ent.push_str(&self.zsh_hint_suffix(&self.resolve_pos_hint(p)));
            ent.push('\'');
            pushln(&mut s, &format!("  {} \\", ent));
        }
        s
    }

    fn zsh_hint_suffix(&self, hint: &ValueHint) -> String {
        match hint {
            ValueHint::Any => "_files -".to_string(),
            ValueHint::File => "_files".to_string(),
            ValueHint::Directory => "_files -/'.'".to_string(),
            ValueHint::Command => "_command_names".to_string(),
            ValueHint::Choice(v) => format!("({})", v.join(" ")),
        }
    }

    /* ---------------------------- FISH ---------------------------- */

    fn gen_fish(&self) -> String {
        let mut s = String::new();
        let name = &self.spec.bin;
        self.fish_cmd(&mut s, &self.spec, name.to_string());
        s
    }

    fn fish_cmd(&self, out: &mut String, cmd: &CmdSpec, bin: String) {
        for o in &cmd.options {
            if o.hidden { continue; }
            let mut line = format!("complete -c {}", bin);
            if let Some(l) = &o.long { line.push_str(&format!(" -l {}", l)); }
            if let Some(c) = o.short { line.push_str(&format!(" -s {}", c)); }
            if let Some(h) = &o.help { line.push_str(&format!(" -d '{}'", fish_escape(h))); }
            if o.takes_value {
                line.push_str(" -r");
                match self.resolve_opt_hint(o) {
                    ValueHint::Any => {},
                    ValueHint::File => line.push_str(" -a '(commandline -ct | path filter -f)'"),
                    ValueHint::Directory => line.push_str(" -a '(commandline -ct | path filter -d)'"),
                    ValueHint::Command => line.push_str(" -a '(command -sq (commandline -ct))'"),
                    ValueHint::Choice(v) => {
                        line.push_str(" -a \"");
                        line.push_str(&v.join(" "));
                        line.push('"');
                    }
                }
            }
            pushln(out, &line);
        }
        // Positionnels: Fish n'a pas de notion directe; souvent on expose via `-a` si aucune option détectée.
        if let Some(p) = cmd.positionals.first() {
            let mut line = format!("complete -c {} -f", bin);
            match self.resolve_pos_hint(p) {
                ValueHint::Any => { /* rien */ }
                ValueHint::File => line.push_str(" -a '(path filter -f)'"),
                ValueHint::Directory => line.push_str(" -a '(path filter -d)'"),
                ValueHint::Command => line.push_str(" -a '(command -sq (commandline -ct))'"),
                ValueHint::Choice(v) => {
                    line.push_str(" -a \"");
                    line.push_str(&v.join(" "));
                    line.push('"');
                }
            }
            if let Some(h) = &p.help { line.push_str(&format!(" -d '{}'", fish_escape(h))); }
            pushln(out, &line);
        }
        // Sous-commandes
        for sc in &cmd.subcommands {
            // règle: proposer la sous-commande quand aucun arg positionnel consommé
            let mut line = format!("complete -c {} -n '__fish_use_subcommand' -a {}", bin, sc.bin);
            if let Some(h) = &sc.about { line.push_str(&format!(" -d '{}'", fish_escape(h))); }
            pushln(out, &line);
            // et on génère ses propres complétions
            self.fish_cmd(out, sc, format!("{} {}", bin, sc.bin));
        }
    }

    /* ----------------------- RESOLUTION HINTS ---------------------- */

    fn resolve_opt_hint(&self, o: &OptSpec) -> ValueHint {
        if let Some(l) = &o.long {
            if let Some(h) = self.long_hints.get(l) { return h.clone(); }
        }
        if let Some(c) = o.short {
            if let Some(h) = self.short_hints.get(&c) { return h.clone(); }
        }
        o.hint.clone()
    }
    fn resolve_pos_hint(&self, p: &PosSpec) -> ValueHint {
        if let Some(h) = self.pos_hints.get(&p.name) { return h.clone(); }
        p.hint.clone()
    }
}

/* =============================== UTIL =============================== */

fn pushln(s: &mut String, line: &str) { s.push_str(line); s.push('\n'); }

fn shell_join(items: &[String]) -> String {
    let mut out = String::new();
    for (i, it) in items.iter().enumerate() {
        if i > 0 { out.push(' '); }
        out.push_str(&shell_escape(it));
    }
    out
}

fn shell_escape(s: &str) -> String {
    if s.chars().all(|c| c.is_ascii_alphanumeric() || "-_./".contains(c)) { return s.into(); }
    let mut r = String::new(); r.push('\'');
    for ch in s.chars() {
        if ch == '\'' { r.push_str("'\\''"); } else { r.push(ch); }
    }
    r.push('\''); r
}

fn fish_escape(s: &str) -> String { s.replace('\'', "\\'") }

/* ============== CONVERSION DE vitte-args::Spec (facultative) ============== */

#[cfg(feature = "args-spec")]
fn convert_from_args(spec: &ArgsSpec) -> CmdSpec {
    let mut options = Vec::new();
    let mut positionals = Vec::new();

    for a in &spec.args {
        match a.kind {
            ArgsArgKind::Flag => {
                options.push(OptSpec {
                    name: a.name.clone(),
                    short: a.short,
                    long: a.long.clone(),
                    aliases: a.aliases.clone(),
                    help: a.help.clone(),
                    takes_value: false,
                    repeatable: false,
                    value_name: None,
                    hint: ValueHint::Any,
                    hidden: a.hidden,
                });
            }
            ArgsArgKind::Opt => {
                let mut hint = ValueHint::Any;
                if let Some(choices) = &a.choices {
                    if !choices.is_empty() { hint = ValueHint::Choice(choices.clone()); }
                }
                // heuristique sur value_name
                if let Some(vn) = &a.value_name {
                    let low = vn.to_ascii_lowercase();
                    if low.contains("file") { hint = ValueHint::File; }
                    if low.contains("dir") || low.contains("path") { hint = ValueHint::Directory; }
                }
                options.push(OptSpec {
                    name: a.name.clone(),
                    short: a.short,
                    long: a.long.clone(),
                    aliases: a.aliases.clone(),
                    help: a.help.clone(),
                    takes_value: true,
                    repeatable: a.repeatable,
                    value_name: a.value_name.clone(),
                    hint,
                    hidden: a.hidden,
                });
            }
            ArgsArgKind::Pos => {
                positionals.push(PosSpec {
                    name: a.name.clone(),
                    help: a.help.clone(),
                    variadic: matches!(a.arity, vitte_args::Arity::Many),
                    hint: ValueHint::Any,
                });
            }
        }
    }

    let subcommands = spec.subcommands.iter().map(convert_from_args).collect();

    CmdSpec {
        bin: spec.bin.clone(),
        about: spec.about.clone(),
        options,
        positionals,
        subcommands,
    }
}

/* ================================= TESTS ================================ */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn bash_has_complete_line() {
        let spec = CmdSpec {
            bin: "demo".into(),
            about: None,
            options: vec![
                OptSpec{ name:"verbose".into(), short:Some('v'), long:Some("verbose".into()),
                    aliases:vec![], help:Some("Verbose".into()), takes_value:false, repeatable:false, value_name:None, hint:ValueHint::Any, hidden:false },
                OptSpec{ name:"out".into(), short:None, long:Some("out".into()),
                    aliases:vec![], help:Some("Out file".into()), takes_value:true, repeatable:false, value_name:Some("FILE".into), hint:ValueHint::File, hidden:false },
            ],
            positionals: vec![],
            subcommands: vec![],
        };
        let gen = CompletionGenerator::new(spec);
        let bash = gen.generate(Shell::Bash).unwrap();
        assert!(bash.contains("complete -F _demo_complete demo"));
        assert!(bash.contains("--verbose"));
        assert!(bash.contains("--out"));
    }

    #[test]
    fn zsh_contains_arguments() {
        let spec = CmdSpec {
            bin: "demo".into(), about: None, options: vec![], positionals: vec![
                PosSpec { name:"input".into(), help:None, variadic:false, hint:ValueHint::File }
            ], subcommands: vec![],
        };
        let gen = CompletionGenerator::new(spec);
        let z = gen.generate(Shell::Zsh).unwrap();
        assert!(z.contains("_arguments"));
    }

    #[test]
    fn fish_subcommands() {
        let sub = CmdSpec { bin:"build".into(), about:Some("Build".into()), options:vec![], positionals:vec![], subcommands:vec![] };
        let spec = CmdSpec { bin:"demo".into(), about:None, options:vec![], positionals:vec![], subcommands:vec![sub] };
        let gen = CompletionGenerator::new(spec);
        let f = gen.generate(Shell::Fish).unwrap();
        assert!(f.contains("__fish_use_subcommand"));
        assert!(f.contains("build"));
    }
}