// vitte/rust/crates/vitte_muf/src/model.rs
#![allow(dead_code)]

use vitte_common::err::{ErrCode, VResult, VitteError};

#[cfg(feature = "alloc")]
use alloc::{collections::BTreeMap, string::String, vec::Vec};

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Manifest {
    pub version: String,
    pub workspace: Workspace,
    pub packages: Vec<Package>,
    pub targets: Vec<Target>,
    pub extras: BTreeMap<String, ManifestValue>,
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Workspace {
    pub name: Option<String>,
    pub version: Option<String>,
    pub authors: Vec<String>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Package {
    pub name: String,
    pub version: String,
    pub path: Option<String>,
    pub deps: Vec<String>,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Target {
    pub name: String,
    pub kind: String,
    pub src: Option<String>,
}

#[derive(Clone, Debug, PartialEq)]
pub enum ManifestValue {
    String(String),
    Int(i64),
    List(Vec<ManifestValue>),
}

pub fn lower(ast: Vec<crate::ast::AstItem>) -> VResult<Manifest> {
    // Phase1: lower known blocks: workspace, package, target.
    // Unknown blocks stored in extras (stringly).
    let mut m = Manifest::default();
    m.version = "0.1".into();

    for item in ast {
        match item {
            crate::ast::AstItem::Header(h) => {
                m.version = h.value.version;
            }
            crate::ast::AstItem::Block(b) => {
                match b.value.name.as_str() {
                    "workspace" => lower_workspace(&mut m, &b.value)?,
                    "package" => lower_package(&mut m, &b.value)?,
                    "target" => lower_target(&mut m, &b.value)?,
                    other => {
                        // store raw key/values as extras
                        let mut obj = BTreeMap::new();
                        for e in &b.value.entries {
                            if let crate::ast::Entry::Assign { key, value } = &e.value {
                                obj.insert(key.clone(), lower_value(&value.value));
                            }
                        }
                        m.extras.insert(other.into(), ManifestValue::String(format!("{obj:?}")));
                    }
                }
            }
        }
    }

    Ok(m)
}

fn lower_workspace(m: &mut Manifest, b: &crate::ast::Block) -> VResult<()> {
    for e in &b.entries {
        if let crate::ast::Entry::Assign { key, value } = &e.value {
            match key.as_str() {
                "name" => m.workspace.name = Some(as_string(&value.value)?),
                "version" => m.workspace.version = Some(as_string(&value.value)?),
                "authors" => m.workspace.authors = as_string_list(&value.value)?,
                _ => {}
            }
        }
    }
    Ok(())
}

fn lower_package(m: &mut Manifest, b: &crate::ast::Block) -> VResult<()> {
    let mut name: Option<String> = None;
    let mut version: Option<String> = None;
    let mut path: Option<String> = None;
    let mut deps: Vec<String> = Vec::new();

    for e in &b.entries {
        if let crate::ast::Entry::Assign { key, value } = &e.value {
            match key.as_str() {
                "name" => name = Some(as_string(&value.value)?),
                "version" => version = Some(as_string(&value.value)?),
                "path" => path = Some(as_string(&value.value)?),
                "deps" => deps = as_string_list(&value.value)?,
                _ => {}
            }
        }
    }

    let name = name.ok_or_else(|| VitteError::new(ErrCode::ParseError))?;
    let version = version.ok_or_else(|| VitteError::new(ErrCode::ParseError))?;

    m.packages.push(Package { name, version, path, deps });
    Ok(())
}

fn lower_target(m: &mut Manifest, b: &crate::ast::Block) -> VResult<()> {
    let mut name: Option<String> = None;
    let mut kind: Option<String> = None;
    let mut src: Option<String> = None;

    for e in &b.entries {
        if let crate::ast::Entry::Assign { key, value } = &e.value {
            match key.as_str() {
                "name" => name = Some(as_string(&value.value)?),
                "kind" => kind = Some(as_string(&value.value)?),
                "src" => src = Some(as_string(&value.value)?),
                _ => {}
            }
        }
    }

    let name = name.ok_or_else(|| VitteError::new(ErrCode::ParseError))?;
    let kind = kind.ok_or_else(|| VitteError::new(ErrCode::ParseError))?;

    m.targets.push(Target { name, kind, src });
    Ok(())
}

fn lower_value(v: &crate::ast::Value) -> ManifestValue {
    match v {
        crate::ast::Value::String(s) => ManifestValue::String(s.clone()),
        crate::ast::Value::Int(n) => ManifestValue::Int(*n),
        crate::ast::Value::List(xs) => ManifestValue::List(xs.iter().map(|x| lower_value(&x.value)).collect()),
    }
}

fn as_string(v: &crate::ast::Value) -> VResult<String> {
    match v {
        crate::ast::Value::String(s) => Ok(s.clone()),
        crate::ast::Value::Int(n) => Ok(n.to_string()),
        crate::ast::Value::List(_) => Err(VitteError::new(ErrCode::ParseError)),
    }
}

fn as_string_list(v: &crate::ast::Value) -> VResult<Vec<String>> {
    match v {
        crate::ast::Value::List(xs) => {
            let mut out = Vec::new();
            for x in xs {
                out.push(as_string(&x.value)?);
            }
            Ok(out)
        }
        _ => Err(VitteError::new(ErrCode::ParseError)),
    }
}
