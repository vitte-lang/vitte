//! vitte-spinner — spinners CLI **ultra complet**
//!
//! Ajouts par rapport à la version précédente:
//! - `Spinner::start()` → handle threadé avec `stop/succeed/fail/with_msg`.
//! - `pause()/resume()`, `set_prefix()/set_suffix()`, `elapsed()`.
//! - Cadence de rendu configurable `set_fps()` et throttling automatique.
//! - Largeur terminal simple via `$COLUMNS` (tronque proprement).
//! - Style presets étendus + API builder (`Style::builder()`).
//! - Guard RAII `scope_spinner(...)` pour un spinner auto-stop à la sortie.
//! - Multi-spinners: réécriture avec réimpression en place (compact).
//! - Zéro `unsafe`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, boxed::Box, format};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, time::{Duration, Instant}, io::{self, Write}, sync::{Arc, atomic::{AtomicBool, Ordering}}, thread, env};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="ansi")]
use vitte_ansi as ansi;

#[cfg(feature="multi")]
use crossbeam_channel as xch;

/* ============================== erreurs ============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum SpinError {
    #[error("feature missing")]
    FeatureMissing,
    #[error("already stopped")]
    Stopped,
}
#[cfg(not(feature="errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum SpinError { FeatureMissing, Stopped }

pub type Result<T> = core::result::Result<T, SpinError>;

/* ============================== cible dessin ============================== */

pub trait DrawTarget {
    fn draw_line(&mut self, s: &str);
    fn clear_line(&mut self);
    fn hide_cursor(&mut self) {}
    fn show_cursor(&mut self) {}
}

#[cfg(feature="std")]
pub struct StdoutTarget;
#[cfg(feature="std")]
impl DrawTarget for StdoutTarget {
    fn draw_line(&mut self, s: &str) {
        let _ = write!(io::stdout(), "\r{}", s);
        let _ = io::stdout().flush();
    }
    fn clear_line(&mut self) {
        let _ = write!(io::stdout(), "\r\x1B[2K");
        let _ = io::stdout().flush();
    }
    fn hide_cursor(&mut self) { let _ = write!(io::stdout(), "\x1B[?25l"); let _ = io::stdout().flush(); }
    fn show_cursor(&mut self) { let _ = write!(io::stdout(), "\x1B[?25h"); let _ = io::stdout().flush(); }
}

/* ============================== Style ============================== */

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Style {
    pub frames: Vec<String>,
    pub interval: Duration,
    pub colorize: bool,
    pub succeed_symbol: String,
    pub fail_symbol: String,
    pub prefix: String,
    pub suffix: String,
    pub truncate: bool,
}
impl Default for Style {
    fn default() -> Self {
        Self {
            frames: preset::dots(),
            interval: Duration::from_millis(80),
            colorize: true,
            succeed_symbol: "✔".to_string(),
            fail_symbol: "✘".to_string(),
            prefix: String::new(),
            suffix: String::new(),
            truncate: true,
        }
    }
}
impl Style {
    pub fn builder() -> StyleBuilder { StyleBuilder::default() }
}
#[derive(Default)]
pub struct StyleBuilder {
    st: Style
}
impl StyleBuilder {
    pub fn frames<I,S>(mut self, it:I)->Self where I:IntoIterator<Item=S>, S:Into<String>{ self.st.frames=it.into_iter().map(Into::into).collect(); self }
    pub fn interval(mut self, d:Duration)->Self{ self.st.interval=d; self }
    pub fn colorize(mut self, on:bool)->Self{ self.st.colorize=on; self }
    pub fn succeed_symbol(mut self, s:impl Into<String>)->Self{ self.st.succeed_symbol=s.into(); self }
    pub fn fail_symbol(mut self, s:impl Into<String>)->Self{ self.st.fail_symbol=s.into(); self }
    pub fn prefix(mut self, s:impl Into<String>)->Self{ self.st.prefix=s.into(); self }
    pub fn suffix(mut self, s:impl Into<String>)->Self{ self.st.suffix=s.into(); self }
    pub fn truncate(mut self, on:bool)->Self{ self.st.truncate=on; self }
    pub fn build(self)->Style{ self.st }
}

/* ============================== Spinner ============================== */

pub struct Spinner {
    style: Style,
    idx: usize,
    text: String,
    started: Option<Instant>,
    stopped: bool,
    paused: bool,
    fps: u32,
    last_draw: Option<Instant>,
    width: Option<usize>,
    #[cfg(feature="std")] target: Box<dyn DrawTarget + Send>,
}
impl Default for Spinner { fn default() -> Self { Self::new() } }
impl Spinner {
    pub fn new() -> Self {
        Self {
            style: Style::default(),
            idx: 0,
            text: String::new(),
            started: None,
            stopped: false,
            paused: false,
            fps: 0,
            last_draw: None,
            width: None,
            #[cfg(feature="std")] target: Box::new(StdoutTarget),
        }
    }
    pub fn frames<I,S>(mut self, it:I)->Self where I:IntoIterator<Item=S>, S:Into<String> {
        self.style.frames = it.into_iter().map(Into::into).collect();
        if self.style.frames.is_empty() { self.style.frames.push("|".into()); }
        self
    }
    pub fn interval(mut self, d:Duration)->Self{ self.style.interval=d; self }
    pub fn set_fps(&mut self, fps:u32){ self.fps=fps.min(120); }
    pub fn colorize(mut self, on:bool)->Self{ self.style.colorize=on; self }
    pub fn success_symbol(mut self, s:impl Into<String>)->Self{ self.style.succeed_symbol=s.into(); self }
    pub fn fail_symbol(mut self, s:impl Into<String>)->Self{ self.style.fail_symbol=s.into(); self }
    pub fn text(mut self, t:impl Into<String>)->Self{ self.text=t.into(); self }
    pub fn set_text(&mut self, t:impl Into<String>){ self.text=t.into(); }
    pub fn set_prefix(&mut self, p:impl Into<String>){ self.style.prefix=p.into(); }
    pub fn set_suffix(&mut self, s:impl Into<String>){ self.style.suffix=s.into(); }
    pub fn pause(&mut self){ self.paused=true; }
    pub fn resume(&mut self){ self.paused=false; }
    pub fn elapsed(&self)->Option<Duration>{ self.started.map(|t| t.elapsed()) }
    pub fn set_width(&mut self, w:Option<usize>){ self.width=w; }

    #[cfg(feature="std")]
    pub fn with_target<T:DrawTarget+Send+'static>(mut self, t:T)->Self{ self.target=Box::new(t); self }
    #[cfg(feature="std")]
    pub fn tick(&mut self) {
        if self.stopped || self.paused { sleep_for(self.style.interval); return; }
        let now = Instant::now();
        if self.started.is_none() { self.started=Some(now); self.target.hide_cursor(); }
        if let Some(last)=self.last_draw {
            if self.fps>0 {
                let min_dt = Duration::from_secs_f64(1.0 / self.fps as f64);
                if now.duration_since(last) < min_dt { return; }
            }
        }
        self.last_draw = Some(now);
        let frame = &self.style.frames[self.idx % self.style.frames.len()];
        self.idx = self.idx.wrapping_add(1);
        let line = self.compose_line(frame, &self.text);
        self.target.draw_line(&line);
        sleep_for(self.style.interval);
    }
    #[cfg(feature="std")]
    pub fn stop_with(&mut self, msg:impl Into<String>) {
        if self.stopped { return; }
        self.stopped=true;
        self.target.clear_line();
        let _ = writeln!(io::stdout(), "{}", truncate_if_needed(self.width_or_env(), &msg.into(), self.style.truncate));
        self.target.show_cursor();
    }
    #[cfg(feature="std")]
    pub fn succeed(&mut self, msg:impl Into<String>) {
        let m = msg.into();
        let symbol = self.paint_ok(&self.style.succeed_symbol);
        self.stop_with(format!("{} {}", symbol, m));
    }
    #[cfg(feature="std")]
    pub fn fail(&mut self, msg:impl Into<String>) {
        let m = msg.into();
        let symbol = self.paint_err(&self.style.fail_symbol);
        self.stop_with(format!("{} {}", symbol, m));
    }
    #[cfg(feature="std")]
    pub fn start(self) -> SpinnerHandle {
        SpinnerHandle::start(self)
    }
    #[cfg(feature="std")]
    fn compose_line(&self, frame:&str, text:&str)->String{
        let w = self.width_or_env();
        let mut s = String::new();
        if !self.style.prefix.is_empty() { s.push_str(&self.style.prefix); s.push(' '); }
        s.push_str(&self.paint_frame(frame));
        if !text.is_empty() { s.push(' '); s.push_str(text); }
        if !self.style.suffix.is_empty() { s.push(' '); s.push_str(&self.style.suffix); }
        truncate_if_needed(w, &s, self.style.truncate)
    }
    #[cfg(feature="std")]
    fn width_or_env(&self)->usize{
        if let Some(w)=self.width { return w; }
        if let Ok(c)=env::var("COLUMNS"){ if let Ok(n)=c.parse::<usize>() { return n.max(20); } }
        80
    }
    #[cfg(feature="std")]
    fn paint_frame(&self, f:&str)->String{
        if cfg!(feature="ansi") && self.style.colorize {
            #[cfg(feature="ansi")] { return ansi::cyan(f).to_string(); }
        }
        f.to_string()
    }
    #[cfg(feature="std")]
    fn paint_ok(&self, s:&str)->String{
        if cfg!(feature="ansi") && self.style.colorize {
            #[cfg(feature="ansi")] { return ansi::green(s).bold().to_string(); }
        }
        s.to_string()
    }
    #[cfg(feature="std")]
    fn paint_err(&self, s:&str)->String{
        if cfg!(feature="ansi") && self.style.colorize {
            #[cfg(feature="ansi")] { return ansi::red(s).bold().to_string(); }
        }
        s.to_string()
    }
}

/* ============================== Handle threadé ============================== */

#[cfg(feature="std")]
pub struct SpinnerHandle {
    stop: Arc<AtomicBool>,
    join: Option<thread::JoinHandle<Spinner>>,
}
#[cfg(feature="std")]
impl SpinnerHandle {
    pub fn start(mut sp: Spinner) -> Self {
        let flag = Arc::new(AtomicBool::new(false));
        let f2 = flag.clone();
        let join = thread::spawn(move || {
            while !f2.load(Ordering::SeqCst) { sp.tick(); }
            sp.target.show_cursor();
            sp
        });
        Self { stop: flag, join: Some(join) }
    }
    pub fn stop(mut self) -> Spinner {
        self.stop.store(true, Ordering::SeqCst);
        if let Some(j)=self.join.take(){ return j.join().unwrap_or_else(|_| Spinner::new()); }
        Spinner::new()
    }
    pub fn with_msg(self, msg:&str) {
        let mut sp = self.stop();
        sp.stop_with(msg.to_string());
    }
    pub fn succeed(self, msg:&str){
        let mut sp=self.stop();
        sp.succeed(msg.to_string());
    }
    pub fn fail(self, msg:&str){
        let mut sp=self.stop();
        sp.fail(msg.to_string());
    }
}

/* ============================== Scope RAII ============================== */

/// Exécute `f` sous un spinner. En cas d’`Ok`, affiche succès; sinon échec.
/// Renvoie le résultat de `f`.
#[cfg(feature="std")]
pub fn scope_spinner<T, F: FnOnce() -> T>(text:&str, on_ok:&str, on_err:&str, f:F) -> T
where T: core::fmt::Debug
{
    let mut sp = Spinner::new().text(text.to_string());
    let h = SpinnerHandle::start(sp);
    let res = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f));
    match res {
        Ok(v) => { h.succeed(on_ok); v }
        Err(_) => { h.fail(on_err); std::panic::resume_unwind(Box::new("scope_spinner panic")); }
    }
}

/* ============================== Multi spinner ============================== */

#[cfg(feature="multi")]
pub struct MultiSpinner {
    tx: xch::Sender<Msg>,
    join: Option<thread::JoinHandle<()>>,
}
#[cfg(feature="multi")]
enum Msg {
    New(String, Style, String),
    Text(String, String),
    Tick,
    Succeed(String, String),
    Fail(String, String),
    Quit,
}
#[cfg(feature="multi")]
impl MultiSpinner {
    pub fn new() -> Self {
        let (tx, rx) = xch::unbounded::<Msg>();
        let join = thread::spawn(move || painter(rx));
        Self { tx, join: Some(join) }
    }
    pub fn add(&self, id: impl Into<String>, style: Style, text: impl Into<String>) {
        let _ = self.tx.send(Msg::New(id.into(), style, text.into()));
    }
    pub fn set_text(&self, id: &str, text: &str) { let _ = self.tx.send(Msg::Text(id.to_string(), text.to_string())); }
    pub fn tick(&self) { let _ = self.tx.send(Msg::Tick); }
    pub fn succeed(&self, id: &str, msg: &str) { let _ = self.tx.send(Msg::Succeed(id.to_string(), msg.to_string())); }
    pub fn fail(&self, id: &str, msg: &str) { let _ = self.tx.send(Msg::Fail(id.to_string(), msg.to_string())); }
    pub fn finish(mut self) {
        let _ = self.tx.send(Msg::Quit);
        if let Some(j) = self.join.take() { let _ = j.join(); }
    }
}
#[cfg(feature="multi")]
fn painter(rx: xch::Receiver<Msg>) {
    use std::collections::BTreeMap;
    let mut order: Vec<String> = Vec::new();
    let mut state: BTreeMap<String,(Style,usize,String)> = BTreeMap::new();
    let mut last = Instant::now();
    let _ = write!(io::stdout(), "\x1B[?25l");
    loop {
        while let Ok(msg) = rx.try_recv() {
            match msg {
                Msg::New(id, st, txt) => { order.push(id.clone()); state.insert(id, (st,0,txt)); }
                Msg::Text(id, txt) => if let Some(s)=state.get_mut(&id){ s.2 = txt; }
                Msg::Tick => {}
                Msg::Succeed(id, msg) => {
                    if let Some((st,_,_))=state.remove(&id){
                        println!("\r\x1B[2K{} {}", paint_ok(&st, &st.succeed_symbol), msg);
                        order.retain(|x| x!=&id);
                    }
                }
                Msg::Fail(id, msg) => {
                    if let Some((st,_,_))=state.remove(&id){
                        println!("\r\x1B[2K{} {}", paint_err(&st, &st.fail_symbol), msg);
                        order.retain(|x| x!=&id);
                    }
                }
                Msg::Quit => { let _=write!(io::stdout(), "\x1B[?25h"); let _=io::stdout().flush(); return; }
            }
        }
        if last.elapsed() >= Duration::from_millis(80) {
            for id in &order {
                if let Some((st,idx,txt)) = state.get_mut(id) {
                    *idx = idx.wrapping_add(1);
                    let frame = &st.frames[*idx % st.frames.len()];
                    println!("\r\x1B[2K{} {}", paint_frame(st, frame), txt);
                }
            }
            last = Instant::now();
            let _ = io::stdout().flush();
        }
        thread::sleep(Duration::from_millis(16));
    }
}
#[cfg(feature="multi")]
fn paint_frame(st:&Style, f:&str)->String{
    if cfg!(feature="ansi") && st.colorize {
        #[cfg(feature="ansi")] { return ansi::cyan(f).to_string(); }
    }
    f.to_string()
}
#[cfg(feature="multi")]
fn paint_ok(st:&Style, s:&str)->String{
    if cfg!(feature="ansi") && st.colorize {
        #[cfg(feature="ansi")] { return ansi::green(s).bold().to_string(); }
    }
    s.to_string()
}
#[cfg(feature="multi")]
fn paint_err(st:&Style, s:&str)->String{
    if cfg!(feature="ansi") && st.colorize {
        #[cfg(feature="ansi")] { return ansi::red(s).bold().to_string(); }
    }
    s.to_string()
}

/* ============================== Presets ============================== */

pub mod preset {
    pub fn dots() -> Vec<String> {
        vec!["⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"].into_iter().map(str::to_string).collect()
    }
    pub fn line() -> Vec<String> { vec!["|","/","-","\\"].into_iter().map(str::to_string).collect() }
    pub fn arrow() -> Vec<String> { vec!["→","↘","↓","↙","←","↖","↑","↗"].into_iter().map(str::to_string).collect() }
    pub fn bounce() -> Vec<String> { vec!["▖","▘","▝","▗"].into_iter().map(str::to_string).collect() }
    pub fn dotline() -> Vec<String> { vec![".  ",".. ","..."," ..","  .","   "].into_iter().map(str::to_string).collect() }
    pub fn triangle() -> Vec<String> { vec!["▲","▶","▼","◀"].into_iter().map(str::to_string).collect() }
    pub fn clock() -> Vec<String> { vec!["🕐","🕑","🕒","🕓","🕔","🕕","🕖","🕗","🕘","🕙","🕚","🕛"].into_iter().map(str::to_string).collect() }
    pub fn bullets() -> Vec<String> { vec!["•","◦","·","∙","●","○"].into_iter().map(str::to_string).collect() }
}

/* ============================== Helpers ============================== */

#[cfg(feature="std")]
fn truncate_if_needed(width: usize, s:&str, on:bool)->String{
    if !on { return s.to_string(); }
    if s.len() <= width.saturating_sub(1) { return s.to_string(); }
    let cut = width.saturating_sub(2).max(0);
    let mut out = s.chars().take(cut).collect::<String>();
    out.push('…');
    out
}
#[cfg(feature="std")]
fn sleep_for(d:Duration){ std::thread::sleep(d); }

/* ============================== Tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn presets_ok(){ assert!(preset::dots().len()>=8); assert_eq!(preset::line().len(),4); }
    #[test] fn style_builder(){ let st=Style::builder().prefix("[x]").suffix("ok").truncate(true).build(); assert_eq!(st.prefix,"[x]"); assert_eq!(st.suffix,"ok"); }
    #[cfg(feature="std")]
    #[test] fn truncation(){ let s=truncate_if_needed(5,"abcdef",true); assert!(s.ends_with('…')); }
}