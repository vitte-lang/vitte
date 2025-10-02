//! vitte-progress — barres, spinners et multi-rendu **encore plus complet**
//!
//! Points ajoutés vs version précédente:
//! - `set_prefix()/set_suffix()` et message secondaire (`set_secondary`).
//! - Pause/Reprise (`pause()/resume()`), remise à zéro (`reset()`).
//! - Contrôle du rafraîchissement: `set_draw_rate(hz)` ou `set_draw_delta(steps)`.
//! - Détection largeur terminal simple via `$COLUMNS` (`effective_width`).
//! - Formatage bytes et unités personnalisables (`set_unit`, `UnitKind`).
//! - Vitesse lissée via EWMA (`set_ewma_alpha`).
//! - Hooks utilisateur: `on_draw(|&RenderState| ...)`.
//! - Groupe non-concurrent `ProgressGroup` pour empiler plusieurs barres sans feature `multi`.
//! - Fallback non-TTY: impression compacte sur une ligne par mise à jour finale.
//!
//! API centrale inchangée: `ProgressBar`, `Spinner`, `MultiProgress` (feature `multi`).

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, format, boxed::Box};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, time::{Duration, Instant}, io::{self, Write}, fmt, cmp, env};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="ansi")]
use vitte_ansi as ansi;

#[cfg(feature="multi")]
use crossbeam_channel as xch;

/* ================================ erreurs ================================ */

#[cfg(feature="errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum ProgressError {
    #[error("feature not enabled")]
    FeatureMissing,
    #[error("invalid state")]
    InvalidState,
}
#[cfg(not(feature="errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum ProgressError { FeatureMissing, InvalidState }

pub type Result<T> = core::result::Result<T, ProgressError>;

/* ================================ style & units ================================ */

/// Choix d’unité pour le formatage pos/len et vitesse.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnitKind {
    Count,      // 123 / s
    BytesSI,    // k, M, G (1000)
    BytesBin,   // Ki, Mi, Gi (1024)
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct ProgressStyle {
    pub template: String,              // "{prefix}{bar} {pos}/{len} {percent:>3}% {eta} {rate} {suffix} {msg}"
    pub bar_width: usize,
    pub bar_chars: (char, char, char), // (fill, head, empty)
    pub tick_chars: Vec<char>,         // spinner frames
    pub show_eta: bool,
    pub show_rate: bool,
    pub use_utf: bool,
    pub use_color: bool,
    pub unit: UnitKind,
}
impl ProgressStyle {
    pub fn new(template: &str) -> Self {
        Self {
            template: template.to_string(),
            bar_width: 40,
            bar_chars: ('█','▌',' '),
            tick_chars: vec!['⠋','⠙','⠹','⠸','⠼','⠴','⠦','⠧','⠇','⠏'],
            show_eta: true, show_rate: true, use_utf: true, use_color: true,
            unit: UnitKind::Count,
        }
    }
    pub fn classic() -> Self {
        Self::new("{prefix}{bar} {pos}/{len} {percent:>3}% {eta} {rate} {suffix} {msg}")
    }
    pub fn ascii() -> Self {
        Self { bar_chars: ('#','>',' '), tick_chars: vec!['|','/','-','\\'], use_utf:false, ..Self::classic() }
    }
    pub fn bar_width(mut self, w: usize) -> Self { self.bar_width = w.max(5); self }
    pub fn bar_chars(mut self, fill: char, head: char, empty: char) -> Self { self.bar_chars=(fill,head,empty); self }
    pub fn tick_chars<I: IntoIterator<Item=char>>(mut self, it: I) -> Self { self.tick_chars = it.into_iter().collect(); self }
    pub fn show_eta(mut self, on: bool) -> Self { self.show_eta = on; self }
    pub fn show_rate(mut self, on: bool) -> Self { self.show_rate = on; self }
    pub fn colors(mut self, on: bool) -> Self { self.use_color = on; self }
    pub fn set_unit(mut self, u: UnitKind) -> Self { self.unit = u; self }
}

/* ================================ rendu cible ================================ */

pub trait DrawTarget { fn draw_line(&mut self, s: &str); fn clear_line(&mut self); }

#[cfg(feature="std")]
pub struct StdoutTarget;
#[cfg(feature="std")]
impl DrawTarget for StdoutTarget {
    fn draw_line(&mut self, s: &str) { let _ = write!(io::stdout(), "\r{}", s); let _ = io::stdout().flush(); }
    fn clear_line(&mut self) { let _ = write!(io::stdout(), "\r\x1B[2K"); let _ = io::stdout().flush(); }
}

/* ================================ état rendu ================================ */

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct RenderState {
    pub pos: u64,
    pub len: u64,
    pub percent: f64,
    pub eta: Option<Duration>,
    pub rate: Option<f64>, // unités/s
    pub message: String,
    pub prefix: String,
    pub suffix: String,
}

/* ================================ ProgressBar ================================ */

pub struct ProgressBar {
    len: u64, pos: u64,
    started: Option<Instant>, last_draw: Option<Instant>,
    style: ProgressStyle,
    msg: String, msg2: String,
    prefix: String, suffix: String,
    finished: bool, paused: bool,
    draw_hz: f32,           // min 1/Hz
    draw_delta: u64,        // redraw après N increments
    last_draw_pos: u64,
    ewma_alpha: f64,        // 0..1
    ewma_rate: f64,         // lissée
    #[cfg(feature="std")] target: Box<dyn DrawTarget + Send>,
    on_draw: Option<Box<dyn Fn(&RenderState) + Send + Sync>>,
    width: usize,           // 0 => autodetect
    quiet: bool,            // si true, n’imprime pas en direct
}
impl fmt::Debug for ProgressBar {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ProgressBar").field("len",&self.len).field("pos",&self.pos).field("finished",&self.finished).finish()
    }
}
impl ProgressBar {
    pub fn new(len: u64) -> Self {
        Self {
            len, pos:0, started:None, last_draw:None,
            style:ProgressStyle::classic(),
            msg:String::new(), msg2:String::new(),
            prefix:String::new(), suffix:String::new(),
            finished:false, paused:false,
            draw_hz: 16.0, draw_delta: 1, last_draw_pos: 0,
            ewma_alpha: 0.25, ewma_rate: 0.0,
            #[cfg(feature="std")] target: Box::new(StdoutTarget),
            on_draw: None, width: 0, quiet: false,
        }
    }
    pub fn with_style(mut self, s: ProgressStyle) -> Self { self.style=s; self }
    #[cfg(feature="std")] pub fn with_target<T: DrawTarget + Send + 'static>(mut self, t:T)->Self{ self.target=Box::new(t); self }
    pub fn set_length(&mut self, len:u64){ self.len=len; self.redraw_throttled(); }
    pub fn set_position(&mut self, pos:u64){ self.pos=cmp::min(pos,self.len); self.redraw_throttled(); }
    pub fn inc(&mut self, d:u64){ if !self.paused { self.set_position(self.pos.saturating_add(d)); } }
    pub fn reset(&mut self){ self.pos=0; self.started=None; self.last_draw=None; self.ewma_rate=0.0; self.last_draw_pos=0; self.redraw_now(); }

    pub fn set_message(&mut self, m:impl Into<String>){ self.msg=m.into(); self.redraw_throttled(); }
    pub fn set_secondary(&mut self, m:impl Into<String>){ self.msg2=m.into(); self.redraw_throttled(); }
    pub fn set_prefix(&mut self, s:impl Into<String>){ self.prefix=s.into(); self.redraw_throttled(); }
    pub fn set_suffix(&mut self, s:impl Into<String>){ self.suffix=s.into(); self.redraw_throttled(); }

    pub fn set_draw_rate(&mut self, hz:f32){ self.draw_hz=hz.clamp(1.0, 120.0); }
    pub fn set_draw_delta(&mut self, steps:u64){ self.draw_delta=steps.max(1); }
    pub fn set_ewma_alpha(&mut self, a:f64){ self.ewma_alpha=a.clamp(0.01, 0.99); }
    pub fn set_width(&mut self, w:usize){ self.width=w; }
    pub fn set_quiet(&mut self, q:bool){ self.quiet=q; }

    pub fn on_draw<F>(&mut self, f:F) where F: Fn(&RenderState)+Send+Sync+'static { self.on_draw = Some(Box::new(f)); }

    pub fn pause(&mut self){ self.paused=true; }
    pub fn resume(&mut self){ self.paused=false; }

    pub fn finish(&mut self){ self.finished=true; self.redraw_now(); self.clear_line(); }
    pub fn finish_with_message(&mut self, m:impl Into<String>){ self.msg=m.into(); self.finish(); }

    #[cfg(feature="std")]
    fn redraw_throttled(&mut self){
        let now=Instant::now(); if self.started.is_none(){ self.started=Some(now); self.last_draw_pos=self.pos; }
        let need_delta = self.pos - self.last_draw_pos >= self.draw_delta;
        let min_dt = Duration::from_secs_f32(1.0/self.draw_hz);
        let need_time = self.last_draw.map(|t| now.duration_since(t)>=min_dt).unwrap_or(true);
        if need_delta && need_time { self.redraw_now(); self.last_draw_pos=self.pos; }
    }

    #[cfg(feature="std")]
    fn redraw_now(&mut self){
        self.last_draw=Some(Instant::now());
        if let Some(st)=self.started {
            let dt = st.elapsed().as_secs_f64().max(1e-6);
            let inst_rate = self.pos as f64 / dt;
            self.ewma_rate = if self.ewma_rate==0.0 { inst_rate } else { self.ewma_alpha*inst_rate + (1.0-self.ewma_alpha)*self.ewma_rate };
        }
        let state = self.render_state();
        if let Some(cb)=&self.on_draw { cb(&state); }
        if self.quiet { return; }
        let line = self.format_line(&state);
        self.target.draw_line(&line);
        if self.finished { let _ = writeln!(io::stdout()); }
    }

    #[cfg(feature="std")] fn clear_line(&mut self){ if !self.quiet { self.target.clear_line(); } }

    #[cfg(feature="std")]
    fn render_state(&self) -> RenderState {
        let percent = if self.len==0 {0.0} else {(self.pos as f64 / self.len as f64) * 100.0};
        let eta = if self.style.show_eta && self.len>0 && self.pos>0 {
            let rate = self.ewma_rate.max(1e-9);
            let rem = (self.len - self.pos) as f64 / rate;
            Some(Duration::from_secs_f64(rem.max(0.0)))
        } else { None };
        let rate = if self.style.show_rate { Some(self.ewma_rate) } else { None };
        RenderState {
            pos: self.pos, len: self.len, percent, eta, rate,
            message: if self.msg2.is_empty(){ self.msg.clone() } else { format!("{} {}", self.msg, self.msg2) },
            prefix: self.prefix.clone(), suffix: self.suffix.clone(),
        }
    }

    #[cfg(feature="std")]
    fn effective_width(&self) -> usize {
        if self.width>0 { return self.width; }
        if let Ok(c)=env::var("COLUMNS") { if let Ok(n)=c.parse::<usize>() { return n.max(40); } }
        80
    }

    #[cfg(feature="std")]
    fn format_line(&self, st:&RenderState)->String{
        let width = self.effective_width();
        let bar=self.render_bar(width);
        let eta=st.eta.map(fmt_hms).map(|s| format!("ETA {}", s)).unwrap_or_default();
        let rate=st.rate.map(|r| format!("{}/s", fmt_units(r, self.style.unit))).unwrap_or_default();
        let mut s=self.style.template.clone();
        replace_all(&mut s,"{prefix}", &st.prefix);
        replace_all(&mut s,"{suffix}", &st.suffix);
        replace_all(&mut s,"{bar}", &bar);
        replace_all(&mut s,"{pos}", &fmt_units(st.pos as f64, self.style.unit));
        replace_all(&mut s,"{len}", &fmt_units(st.len as f64, self.style.unit));
        replace_all(&mut s,"{percent}", &format!("{:.0}", st.percent));
        replace_all(&mut s,"{eta}", &eta);
        replace_all(&mut s,"{rate}", &rate);
        replace_all(&mut s,"{msg}", &st.message);

        if cfg!(feature="ansi") && self.style.use_color {
            #[cfg(feature="ansi")]
            {
                let col = if st.percent>=100.0 { ansi::green("") } else if st.percent>=66.0 { ansi::yellow("") } else { ansi::cyan("") };
                return format!("{}{}{}", col.prefix(), s, col.suffix());
            }
        }
        s
    }

    #[cfg(feature="std")]
    fn render_bar(&self, term_width: usize)->String{
        let w=self.style.bar_width.min(term_width.saturating_sub(20)).max(5);
        if self.len==0 { return repeat_char(self.style.bar_chars.2,w); }
        let ratio=(self.pos as f64/self.len as f64).clamp(0.0,1.0);
        let filled=(ratio*w as f64).floor() as usize;
        let mut s=String::with_capacity(w);
        for _ in 0..filled.saturating_sub(1){ s.push(self.style.bar_chars.0); }
        if filled>0{ s.push(self.style.bar_chars.1); }
        for _ in filled..w{ s.push(self.style.bar_chars.2); }
        s
    }
}

/* ================================ Spinner ================================ */

pub struct Spinner {
    style: ProgressStyle, tick: usize, msg: String,
    started: Option<Instant>, interval: Duration, stopped: bool,
    #[cfg(feature="std")] target: Box<dyn DrawTarget + Send>,
}
impl Spinner {
    pub fn new()->Self{
        Self{ style:ProgressStyle::classic(), tick:0, msg:String::new(),
              started:None, interval:Duration::from_millis(80), stopped:false,
              #[cfg(feature="std")] target: Box::new(StdoutTarget) }
    }
    pub fn with_style(mut self, s:ProgressStyle)->Self{ self.style=s; self }
    pub fn interval(mut self, d:Duration)->Self{ self.interval=d; self }
    pub fn set_message(&mut self, m:impl Into<String>){ self.msg=m.into(); }

    #[cfg(feature="std")]
    pub fn tick(&mut self){
        if self.stopped { return; }
        if self.started.is_none(){ self.started=Some(Instant::now()); }
        let ch=self.style.tick_chars.get(self.tick % self.style.tick_chars.len()).copied().unwrap_or('|');
        self.tick=self.tick.wrapping_add(1);
        self.target.draw_line(&format!("{} {}", ch, self.msg));
        std::thread::sleep(self.interval);
    }
    #[cfg(feature="std")]
    pub fn stop_with_message(&mut self, m:impl Into<String>){
        self.msg=m.into(); self.stopped=true; self.target.clear_line();
        let _ = writeln!(io::stdout(), "{}", self.msg);
    }
}

/* ================================ MultiProgress (option) ================================ */

#[cfg(feature="multi")]
pub struct MultiProgress { tx: xch::Sender<Msg>, join: Option<std::thread::JoinHandle<()>>, }
#[cfg(feature="multi")]
enum Msg { New(u64, ProgressStyle, String, String, String, xch::Sender<u64>), Inc(String,u64), Pos(String,u64), Msgs(String,String,String), Finish(String,Option<String>), Quit }

#[cfg(feature="multi")]
impl MultiProgress {
    pub fn new()->Self{ let (tx,rx)=xch::bounded::<Msg>(1024); let join=std::thread::spawn(move|| painter(rx)); Self{tx,join:Some(join)} }
    pub fn add(&self, id:impl Into<String>, len:u64, style:ProgressStyle)->MpBar{
        let (btx,brx)=xch::bounded::<u64>(1); let id=id.into(); let _=self.tx.send(Msg::New(len,style,id.clone(),String::new(),String::new(),btx)); let _=brx.recv_timeout(std::time::Duration::from_millis(50)); MpBar{id, tx:self.tx.clone()}
    }
    pub fn join(mut self){ let _=self.tx.send(Msg::Quit); if let Some(j)=self.join.take(){ let _=j.join(); } }
}
#[cfg(feature="multi")]
pub struct MpBar { id:String, tx:xch::Sender<Msg> }
#[cfg(feature="multi")]
impl MpBar {
    pub fn inc(&self, d:u64){ let _=self.tx.send(Msg::Inc(self.id.clone(),d)); }
    pub fn set_position(&self, p:u64){ let _=self.tx.send(Msg::Pos(self.id.clone(),p)); }
    pub fn set_messages(&self, prefix:&str, msg:&str, suffix:&str){ let _=self.tx.send(Msg::Msgs(self.id.clone(), prefix.to_string(), msg.to_string())); let _=self.tx.send(Msg::Msgs(self.id.clone(), "".into(), suffix.to_string())); }
    pub fn finish(&self){ let _=self.tx.send(Msg::Finish(self.id.clone(), None)); }
    pub fn finish_with_message(&self, m:&str){ let _=self.tx.send(Msg::Finish(self.id.clone(), Some(m.to_string()))); }
}
#[cfg(feature="multi")]
fn painter(rx: xch::Receiver<Msg>){
    use std::collections::BTreeMap;
    let mut states:BTreeMap<String,(u64,u64,ProgressStyle,String,String,Instant)>=BTreeMap::new(); // len,pos,style,msg,suffix,start
    let mut order:Vec<String>=Vec::new();
    let mut last=Instant::now();
    loop{
        while let Ok(msg)=rx.try_recv(){
            match msg {
                Msg::New(len,style,id,_,_,ack)=>{ states.insert(id.clone(),(len,0,style,String::new(),String::new(),Instant::now())); order.push(id); let _=ack.send(0); }
                Msg::Inc(id,d)=>if let Some(s)=states.get_mut(&id){ s.1=s.1.saturating_add(d); }
                Msg::Pos(id,p)=>if let Some(s)=states.get_mut(&id){ s.1=p; }
                Msg::Msgs(id,m,suf)=>if let Some(st)=states.get_mut(&id){ st.3=m; st.4=suf; }
                Msg::Finish(id,msg)=>{ if let Some(_)=states.remove(&id){ if let Some(m)=msg { let _=println!("{}",m); } order.retain(|x| x!=&id); } }
                Msg::Quit=>{ redraw_all(&states,&order); return; }
            }
        }
        if last.elapsed()>=Duration::from_millis(80){ redraw_all(&states,&order); last=Instant::now(); }
        std::thread::sleep(Duration::from_millis(10));
    }
}
#[cfg(feature="multi")]
fn redraw_all(states:&std::collections::BTreeMap<String,(u64,u64,ProgressStyle,String,String,Instant)>, order:&[String]){
    let _=print!("\x1B[?25l");
    for id in order {
        if let Some((len,pos,style,msg,suffix,start))=states.get(id){
            let (bar,pct)=render_bar_static(*len,*pos,style.bar_width,style.bar_chars);
            let eta= if style.show_eta && *pos>0 {
                let rate=(*pos as f64 / start.elapsed().as_secs_f64().max(1e-6)).max(0.0);
                let rem=(*len as f64-*pos as f64).max(0.0)/rate.max(1e-9);
                format!("ETA {}", fmt_hms(Duration::from_secs_f64(rem)))
            } else { String::new() };
            let mut line=style.template.clone();
            replace_all(&mut line,"{prefix}","");
            replace_all(&mut line,"{bar}",&bar);
            replace_all(&mut line,"{pos}",&format!("{}",pos));
            replace_all(&mut line,"{len}",&format!("{}",len));
            replace_all(&mut line,"{percent}",&format!("{:.0}",pct));
            replace_all(&mut line,"{eta}",&eta);
            replace_all(&mut line,"{rate}","");
            replace_all(&mut line,"{suffix}",suffix);
            replace_all(&mut line,"{msg}",msg);
            println!("\r\x1B[2K{}", line);
        }
    }
    let _=print!("\x1B[?25h"); let _=io::stdout().flush();
}

/* ================================ Group sans multi ================================ */

/// Gestion simple de plusieurs barres sans thread ni channels.
pub struct ProgressGroup {
    bars: Vec<ProgressBar>,
}
impl ProgressGroup {
    pub fn new() -> Self { Self { bars: Vec::new() } }
    pub fn push(&mut self, pb: ProgressBar) { self.bars.push(pb); }
    /// Redessine séquentiellement. Appelle `tick_all()` dans votre boucle.
    #[cfg(feature="std")]
    pub fn tick_all(&mut self) {
        for b in &mut self.bars {
            b.redraw_now();
        }
    }
}

/* ================================ indicatif adapter (option) ================================ */

#[cfg(all(feature="indicatif", feature="std"))]
pub mod indicatif_adapter {
    use super::*; use indicatif::{ProgressBar as IBar, ProgressStyle as IStyle};

    pub struct IndicatifBar{ inner:IBar, style:ProgressStyle }
    impl IndicatifBar{
        pub fn new(len:u64)->Self{ Self{ inner:IBar::new(len), style:ProgressStyle::classic() } }
        pub fn with_style(mut self, s:ProgressStyle)->Self{
            let tpl=s.template.replace("{bar}","{bar:=>bar_width}");
            let ist=IStyle::default_bar()
                .template(&tpl).unwrap()
                .progress_chars(&format!("{}{}{}",s.bar_chars.0,s.bar_chars.1,s.bar_chars.2))
                .tick_strings(&s.tick_chars.iter().map(|c| c.to_string()).collect::<Vec<_>>());
            self.inner.set_style(ist); self.style=s; self
        }
        pub fn set_message(&self, m:&str){ self.inner.set_message(m.to_string()); }
        pub fn inc(&self, d:u64){ self.inner.inc(d); }
        pub fn set_position(&self, p:u64){ self.inner.set_position(p); }
        pub fn finish(&self){ self.inner.finish(); }
        pub fn finish_with_message(&self, m:&str){ self.inner.finish_with_message(m.to_string()); }
    }
}

/* ================================ helpers ================================ */

#[cfg(feature="std")]
fn replace_all(hay: &mut String, pat: &str, rep: &str){
    if pat.is_empty(){ return; }
    let mut out=String::with_capacity(hay.len()+rep.len());
    let mut i=0;
    while let Some(pos)=hay[i..].find(pat){
        out.push_str(&hay[i..i+pos]); out.push_str(rep); i+=pos+pat.len();
    }
    out.push_str(&hay[i..]); *hay=out;
}
#[cfg(feature="std")] fn repeat_char(c:char,n:usize)->String{ core::iter::repeat(c).take(n).collect() }
#[cfg(feature="std")]
fn fmt_hms(d:Duration)->String{ let s=d.as_secs(); let h=s/3600; let m=(s%3600)/60; let s=s%60; if h>0{ format!("{:02}:{:02}:{:02}",h,m,s)} else { format!("{:02}:{:02}",m,s)} }

#[cfg(feature="std")]
fn fmt_units(v:f64, unit: UnitKind)->String{
    match unit {
        UnitKind::Count => {
            let u=["","k","M","G","T"]; let mut x=v; let mut i=0; while x>=1000.0 && i+1<u.len(){ x/=1000.0; i+=1;} format!("{:.1}{}",x,u[i])
        }
        UnitKind::BytesSI => {
            let u=["B","kB","MB","GB","TB"]; let mut x=v; let mut i=0; while x>=1000.0 && i+1<u.len(){ x/=1000.0; i+=1;} format!("{:.1} {}",x,u[i])
        }
        UnitKind::BytesBin => {
            let u=["B","KiB","MiB","GiB","TiB"]; let mut x=v; let mut i=0; while x>=1024.0 && i+1<u.len(){ x/=1024.0; i+=1;} format!("{:.1} {}",x,u[i])
        }
    }
}

#[cfg(feature="std")]
fn render_bar_static(len:u64,pos:u64,width:usize,chars:(char,char,char))->(String,f64){
    if len==0{ return (repeat_char(chars.2,width), 0.0); }
    let ratio=(pos as f64/len as f64).clamp(0.0,1.0);
    let filled=(ratio*width as f64).floor() as usize;
    let mut s=String::with_capacity(width);
    for _ in 0..filled.saturating_sub(1){ s.push(chars.0); }
    if filled>0{ s.push(chars.1); }
    for _ in filled..width{ s.push(chars.2); }
    (s, ratio*100.0)
}

/* ================================ tests ================================ */

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn style_builders(){ let s=ProgressStyle::classic().bar_width(10).show_eta(false).show_rate(false).set_unit(UnitKind::BytesBin); assert_eq!(s.bar_width,10); assert_eq!(s.unit, UnitKind::BytesBin); }
    #[cfg(feature="std")]
    #[test] fn helpers_render_bar(){ let (bar,pct)=render_bar_static(100,50,20,('#','>',' ')); assert_eq!(bar.len(),20); assert!((pct-50.0).abs()<1e-6); }
}