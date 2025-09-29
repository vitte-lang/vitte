//! vitte-scheduler — orchestration de passes de compilation sous forme de DAG
//!
//! Objectifs :
//! - Déclarer des tâches avec dépendances
//! - Planifier dans un DAG (toposort)
//! - Exécuter séquentiellement, en parallèle (`rayon`) ou async (`tokio`)
//! - Exporter le plan et résultats en JSON si `feature = "json"`
//!
//! API rapide :
//! - [`Task`] : noeud logique
//! - [`Scheduler`] : gestionnaire du DAG
//! - [`Scheduler::execute`] : exécution séquentielle
//! - [`Scheduler::execute_parallel`] : si `rayon`
//! - [`Scheduler::execute_async`] : si `tokio`

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{bail, Context, Result};
use indexmap::IndexMap;
use std::collections::{HashMap, HashSet};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Identifiant de tâche.
pub type TaskId = String;

/// Une tâche du scheduler.
#[derive(Clone)]
pub struct Task {
    pub id: TaskId,
    pub deps: Vec<TaskId>,
    pub run: fn() -> Result<()>,
}

impl Task {
    pub fn new(id: impl Into<String>, run: fn() -> Result<()>) -> Self {
        Self { id: id.into(), deps: Vec::new(), run }
    }
    pub fn with_deps(mut self, deps: Vec<TaskId>) -> Self {
        self.deps = deps;
        self
    }
}

/// Résultat d’exécution.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct TaskResult {
    pub id: TaskId,
    pub success: bool,
    pub error: Option<String>,
}

/// Planificateur.
pub struct Scheduler {
    tasks: IndexMap<TaskId, Task>,
}

impl Scheduler {
    pub fn new() -> Self {
        Self { tasks: IndexMap::new() }
    }

    pub fn add(&mut self, t: Task) {
        self.tasks.insert(t.id.clone(), t);
    }

    /// Dépendances valides ?
    fn check_deps(&self) -> Result<()> {
        for (id, t) in &self.tasks {
            for d in &t.deps {
                if !self.tasks.contains_key(d) {
                    bail!("tâche {id} dépend de {d} inexistant");
                }
            }
        }
        Ok(())
    }

    /// Tri topologique simple.
    fn topo_order(&self) -> Result<Vec<TaskId>> {
        self.check_deps()?;
        let mut indeg: HashMap<&TaskId, usize> =
            self.tasks.iter().map(|(k, _)| (k, 0usize)).collect();
        for (_, t) in &self.tasks {
            for d in &t.deps {
                *indeg.get_mut(d).unwrap() += 1;
            }
        }
        let mut q: Vec<TaskId> = indeg.iter().filter(|(_, &v)| v == 0).map(|(k, _)| (*k).clone()).collect();
        let mut order = Vec::new();
        let mut indeg_mut = indeg.clone();

        while let Some(n) = q.pop() {
            order.push(n.clone());
            if let Some(t) = self.tasks.get(&n) {
                for d in &t.deps {
                    let c = indeg_mut.get_mut(d).unwrap();
                    *c -= 1;
                    if *c == 0 {
                        q.push(d.clone());
                    }
                }
            }
        }
        if order.len() != self.tasks.len() {
            bail!("cycle détecté dans le DAG des tâches");
        }
        Ok(order)
    }

    /// Exécution séquentielle.
    pub fn execute(&self) -> Result<Vec<TaskResult>> {
        let order = self.topo_order()?;
        let mut out = Vec::new();
        for id in order {
            let task = self.tasks.get(&id).unwrap();
            let r = (task.run)();
            match r {
                Ok(_) => out.push(TaskResult { id, success: true, error: None }),
                Err(e) => out.push(TaskResult { id, success: false, error: Some(e.to_string()) }),
            }
        }
        Ok(out)
    }

    /// Exécution parallèle (ordre topo respecté).
    #[cfg(feature = "rayon")]
    pub fn execute_parallel(&self) -> Result<Vec<TaskResult>> {
        use rayon::prelude::*;
        self.check_deps()?;
        // strat: dépendances doivent être prêtes → pour simplification, séquentiel par niveaux
        let mut indeg: HashMap<TaskId, usize> =
            self.tasks.iter().map(|(k, _)| (k.clone(), 0usize)).collect();
        for (_, t) in &self.tasks {
            for d in &t.deps {
                *indeg.get_mut(d).unwrap() += 1;
            }
        }

        let mut out = Vec::new();
        let mut done: HashSet<TaskId> = HashSet::new();
        while done.len() < self.tasks.len() {
            let ready: Vec<&Task> = self
                .tasks
                .values()
                .filter(|t| !done.contains(&t.id) && t.deps.iter().all(|d| done.contains(d)))
                .collect();
            if ready.is_empty() {
                bail!("deadlock/cycle détecté");
            }
            let results: Vec<TaskResult> = ready
                .par_iter()
                .map(|t| {
                    let r = (t.run)();
                    match r {
                        Ok(_) => TaskResult { id: t.id.clone(), success: true, error: None },
                        Err(e) => TaskResult { id: t.id.clone(), success: false, error: Some(e.to_string()) },
                    }
                })
                .collect();
            for r in results {
                done.insert(r.id.clone());
                out.push(r);
            }
        }
        Ok(out)
    }

    /// Exécution asynchrone (tokio).
    #[cfg(feature = "async")]
    pub async fn execute_async(&self) -> Result<Vec<TaskResult>> {
        self.check_deps()?;
        let order = self.topo_order()?;
        let mut out = Vec::new();
        for id in order {
            let task = self.tasks.get(&id).unwrap();
            // run est sync, donc spawn_blocking
            let id_clone = id.clone();
            let fut = tokio::task::spawn_blocking(move || (task.run)());
            match fut.await.unwrap() {
                Ok(_) => out.push(TaskResult { id: id_clone, success: true, error: None }),
                Err(e) => out.push(TaskResult { id: id_clone, success: false, error: Some(e.to_string()) }),
            }
        }
        Ok(out)
    }

    /// Export du plan en JSON (id + deps).
    #[cfg(feature = "json")]
    pub fn export_plan_json(&self) -> Result<String> {
        #[derive(Serialize)]
        struct JsonTask<'a> {
            id: &'a str,
            deps: &'a [TaskId],
        }
        let plan: Vec<_> = self
            .tasks
            .values()
            .map(|t| JsonTask { id: &t.id, deps: &t.deps })
            .collect();
        Ok(serde_json::to_string_pretty(&plan)?)
    }
}

/* =============================== Tests =============================== */

#[cfg(test)]
mod tests {
    use super::*;

    fn mk_task(id: &str) -> Task {
        Task::new(id, || Ok(()))
    }

    #[test]
    fn seq_exec_works() {
        let mut s = Scheduler::new();
        s.add(mk_task("a"));
        s.add(mk_task("b").with_deps(vec!["a".into()]));
        let r = s.execute().unwrap();
        assert_eq!(r.len(), 2);
        assert!(r[0].success);
    }

    #[cfg(feature = "rayon")]
    #[test]
    fn parallel_exec_works() {
        let mut s = Scheduler::new();
        s.add(mk_task("a"));
        s.add(mk_task("b"));
        let r = s.execute_parallel().unwrap();
        assert_eq!(r.len(), 2);
    }

    #[cfg(feature = "json")]
    #[test]
    fn export_json() {
        let mut s = Scheduler::new();
        s.add(mk_task("a"));
        let j = s.export_plan_json().unwrap();
        assert!(j.contains("a"));
    }
}