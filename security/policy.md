//! modules/random.vitte
//! -------------------------------------------------------------
//! Random — PRNG sobres et rapides (XorShift64* & PCG32), helpers uniformes,
//! tirages pondérés, shuffle, chaînes aléatoires, loi normale/exponentielle.
//!
//! État : stable (🖥🛠🔌🧪🌐)
//!
//! Design:
//!   - Deux PRNG :
//!       • XorShift64* : ultra simple, 64-bit, état 64 bits (non-cryptographique)
//!       • PCG32       : 64→32 XSH-RR (Melissa O’Neill), excellent 32-bit
//!   - Seed : `from_seed(u64)` ou `from_entropy()` (mix d’entropie/horloge).
//!   - Uniformes sans biais (rejection sampling).
//!   - `next_f64()` ∈ [0,1), `uniform_*`, `bernoulli(p)`.
//!   - `shuffle`, `choose`, `sample_k`, `weighted_index`.
//!   - `normal(mean, std)` (Box–Muller, avec cache), `exp(lambda)`.
//!   - Génération octets / chaîne base62.
//!
//! ⚠️ Sécurité : ces PRNG **ne sont pas** cryptographiquement sûrs.
//!
//! Licence : MIT

use string
use mathx

// -------------------------------------------------------------
// Entropie & utilitaires bas-niveau
// -------------------------------------------------------------

extern(c) do __vitte_entropy64() -> u64          // peut retourner 0 si indisponible
extern(c) do __vitte_now_ns() -> u64              // horloge monotone/haute résolution

pub const U64_MAX : u64 = 18446744073709551615;
const GOLDEN_GAMMA : u64 = 0x9E3779B97F4A7C15;    // 2^64 / φ
const INV_2POW53   : f64 = 1.0 / 9007199254740992.0; // 2^-53

// splitmix64 — mélange rapide pour dériver états à partir d’un seed
inline do splitmix64(mut x: u64) -> u64 {
  x = x + 0x9E3779B97F4A7C15;
  let mut z = x;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
  z ^ (z >> 31)
}

inline do nonzero_seed64(s: u64) -> u64 {
  if s == 0 { GOLDEN_GAMMA } else { s }
}

pub do entropy64() -> u64 {
  let e = __vitte_entropy64()
  if e != 0 { return e }
  // fallback pauvre mais déterministe localement : temps mélangé
  splitmix64(__vitte_now_ns() ^ 0xD1B54A32D192ED03)
}

// -------------------------------------------------------------
// XorShift64* (Marsaglia) — état 64 bits
// -------------------------------------------------------------

pub struct RngXorShift64 {
  state: u64,
  // cache pour Box–Muller (normale)
  has_spare: bool,
  spare: f64,
}

pub do xorshift64_from_seed(seed: u64) -> RngXorShift64 {
  RngXorShift64{ state: nonzero_seed64(splitmix64(seed)), has_spare: false, spare: 0.0 }
}
pub do xorshift64_from_entropy() -> RngXorShift64 {
  xorshift64_from_seed(entropy64())
}

pub do next_u64(self &mut RngXorShift64) -> u64 {
  // xorshift64* : (x ^= x >> 12; x ^= x << 25; x ^= x >> 27; return x * 2685821657736338717)
  let mut x = self.state
  x ^= (x >> 12)
  x ^= (x << 25)
  x ^= (x >> 27)
  self.state = x
  x * 2685821657736338717
}

pub do next_u32(self &mut RngXorShift64) -> u32 {
  (self.next_u64() >> 32) as u32
}

pub do next_f64(self &mut RngXorShift64) -> f64 {
  // 53 bits de mantisse aléatoires → [0,1)
  let r = self.next_u64() >> 11
  (r as f64) * INV_2POW53
}

pub do fill_bytes(self &mut RngXorShift64, out &mut []u8) {
  let mut i: usize = 0
  let n = out.len()
  while i + 8 <= n {
    let w = self.next_u64()
    out[i+0] = (w       & 0xFF) as u8
    out[i+1] = ((w>>8)  & 0xFF) as u8
    out[i+2] = ((w>>16) & 0xFF) as u8
    out[i+3] = ((w>>24) & 0xFF) as u8
    out[i+4] = ((w>>32) & 0xFF) as u8
    out[i+5] = ((w>>40) & 0xFF) as u8
    out[i+6] = ((w>>48) & 0xFF) as u8
    out[i+7] = ((w>>56) & 0xFF) as u8
    i += 8
  }
  if i < n {
    let w = self.next_u64()
    let mut k: usize = 0
    while i < n {
      out[i] = ((w >> (8*k)) & 0xFF) as u8
      i += 1; k += 1
    }
  }
}

// -------------------------------------------------------------
// PCG32 — 64→32 XSH RR, état 64 bits + inc impair
// -------------------------------------------------------------

pub struct RngPcg32 {
  state: u64,
  inc: u64,          // doit être impair (2n+1)
  has_spare: bool,
  spare: f64,
}

pub do pcg32_from_seed(seed: u64, seq: u64) -> RngPcg32 {
  // initialisation recommandée : state=0; advance avec seed; inc=(seq<<1)|1
  let mut r = RngPcg32{ state: 0, inc: ((seq << 1) | 1), has_spare: false, spare: 0.0 }
  // one step pour diffuser
  r.next_u32()
  r.state += seed
  r.next_u32()
  r
}
pub do pcg32_from_entropy() -> RngPcg32 {
  let s = entropy64()
  let q = splitmix64(s ^ 0xC6BC279692B5CC83)
  pcg32_from_seed(s, q)
}

pub do next_u32(self &mut RngPcg32) -> u32 {
  // state = state * 6364136223846793005 + inc
  let old = self.state
  self.state = old * 6364136223846793005 + self.inc
  // xorshift high, then rotate by top 5 bits
  let xorshifted = (((old >> 18) ^ old) >> 27) as u32
  let rot = (old >> 59) as u32
  (xorshifted >> rot) | (xorshifted << ((-rot) & 31))
}

pub do next_u64(self &mut RngPcg32) -> u64 {
  // composer deux tirages
  ((self.next_u32() as u64) << 32) | (self.next_u32() as u64)
}

pub do next_f64(self &mut RngPcg32) -> f64 {
  let r = (self.next_u64() >> 11) as u64
  (r as f64) * INV_2POW53
}

pub do fill_bytes(self &mut RngPcg32, out &mut []u8) {
  let mut i: usize = 0
  let n = out.len()
  while i + 4 <= n {
    let w = self.next_u32()
    out[i+0] = (w       & 0xFF) as u8
    out[i+1] = ((w>>8)  & 0xFF) as u8
    out[i+2] = ((w>>16) & 0xFF) as u8
    out[i+3] = ((w>>24) & 0xFF) as u8
    i += 4
  }
  while i < n {
    out[i] = (self.next_u32() & 0xFF) as u8
    i += 1
  }
}

// -------------------------------------------------------------
// Uniformes sans biais (bornes exclusives)
// -------------------------------------------------------------

// u64 dans [0, bound) — sans biais (rejection)
pub do u64_below(self &mut RngXorShift64, bound: u64) -> u64 {
  if bound == 0 { return 0 }
  let limit = U64_MAX - (U64_MAX % bound)
  loop {
    let r = self.next_u64()
    if r < limit { return r % bound }
  }
}
pub do u64_below(self &mut RngPcg32, bound: u64) -> u64 {
  if bound == 0 { return 0 }
  let limit = U64_MAX - (U64_MAX % bound)
  loop {
    let r = self.next_u64()
    if r < limit { return r % bound }
  }
}

// i64 dans [min, max] (max exclusif)
pub do i64_range(self &mut RngXorShift64, min: i64, max: i64) -> i64 {
  if max <= min { return min }
  let span = (max as i128) - (min as i128)
  let u = self.u64_below(span as u64)
  (min as i128 + (u as i128)) as i64
}
pub do i64_range(self &mut RngPcg32, min: i64, max: i64) -> i64 {
  if max <= min { return min }
  let span = (max as i128) - (min as i128)
  let u = self.u64_below(span as u64)
  (min as i128 + (u as i128)) as i64
}

// f64 uniformes
pub do f64_range(self &mut RngXorShift64, min: f64, max: f64) -> f64 {
  if !(max > min) { return min }
  min + (max - min) * self.next_f64()
}
pub do f64_range(self &mut RngPcg32, min: f64, max: f64) -> f64 {
  if !(max > min) { return min }
  min + (max - min) * self.next_f64()
}

// Bernoulli(p)
pub do bernoulli(self &mut RngXorShift64, p: f64) -> bool {
  if p <= 0.0 { return false }
  if p >= 1.0 { return true }
  self.next_f64() < p
}
pub do bernoulli(self &mut RngPcg32, p: f64) -> bool {
  if p <= 0.0 { return false }
  if p >= 1.0 { return true }
  self.next_f64() < p
}

// -------------------------------------------------------------
// Shuffle & échantillonnage
// -------------------------------------------------------------

pub do shuffle_in_place[T](self &mut RngXorShift64, xs &mut Vec[T]) {
  let n = xs.len()
  if n < 2 { return }
  let mut i: usize = n - 1
  // Fisher–Yates (Knuth)
  while i > 0 {
    let j = self.u64_below((i as u64) + 1) as usize
    let tmp = xs[i]; xs[i] = xs[j]; xs[j] = tmp
    i -= 1
  }
}
pub do shuffle_in_place[T](self &mut RngPcg32, xs &mut Vec[T]) {
  let n = xs.len()
  if n < 2 { return }
  let mut i: usize = n - 1
  while i > 0 {
    let j = self.u64_below((i as u64) + 1) as usize
    let tmp = xs[i]; xs[i] = xs[j]; xs[j] = tmp
    i -= 1
  }
}

pub do choose_ref[T](self &mut RngXorShift64, xs &Vec[T]) -> Option[&T] {
  if xs.len() == 0 { return None }
  let i = self.u64_below(xs.len() as u64) as usize
  Some(&xs[i])
}
pub do choose_ref[T](self &mut RngPcg32, xs &Vec[T]) -> Option[&T] {
  if xs.len() == 0 { return None }
  let i = self.u64_below(xs.len() as u64) as usize
  Some(&xs[i])
}

// Reservoir sampling (k items, copie les éléments)
pub do sample_k[T](self &mut RngXorShift64, xs &Vec[T], k: usize) -> Vec[T] {
  let n = xs.len()
  let kk = mathx::min_i64(k as i64, n as i64) as usize
  if kk == 0 { return Vec::with_capacity(0) }
  let mut res = Vec::with_capacity(kk)
  // remplissage initial
  let mut i: usize = 0
  while i < kk { res.push(xs[i]); i += 1 }
  // remplacement aléatoire
  while i < n {
    let j = self.u64_below((i as u64) + 1) as usize
    if j < kk { res[j] = xs[i] }
    i += 1
  }
  res
}
pub do sample_k[T](self &mut RngPcg32, xs &Vec[T], k: usize) -> Vec[T] {
  let n = xs.len()
  let kk = mathx::min_i64(k as i64, n as i64) as usize
  if kk == 0 { return Vec::with_capacity(0) }
  let mut res = Vec::with_capacity(kk)
  let mut i: usize = 0
  while i < kk { res.push(xs[i]); i += 1 }
  while i < n {
    let j = self.u64_below((i as u64) + 1) as usize
    if j < kk { res[j] = xs[i] }
    i += 1
  }
  res
}

// Choix pondéré : renvoie l’index selon poids >= 0 (sum>0)
pub do weighted_index(self &mut RngXorShift64, weights: Vec[f64]) -> Option[usize] {
  let mut sum = 0.0
  for w in weights { if w < 0.0 { return None } sum += w }
  if sum <= 0.0 { return None }
  let mut t = self.f64_range(0.0, sum)
  let mut i: usize = 0
  while i < weights.len() {
    if t < weights[i] { return Some(i) }
    t -= weights[i]
    i += 1
  }
  Some(weights.len() - 1) // garde-fou num.
}
pub do weighted_index(self &mut RngPcg32, weights: Vec[f64]) -> Option[usize] {
  let mut sum = 0.0
  for w in weights { if w < 0.0 { return None } sum += w }
  if sum <= 0.0 { return None }
  let mut t = self.f64_range(0.0, sum)
  let mut i: usize = 0
  while i < weights.len() {
    if t < weights[i] { return Some(i) }
    t -= weights[i]
    i += 1
  }
  Some(weights.len() - 1)
}

// -------------------------------------------------------------
// Distributions : normale (Box–Muller), exponentielle
// -------------------------------------------------------------

// Box–Muller avec cache (Z1), met à jour has_spare/spare du RNG

pub do normal(self &mut RngXorShift64, mean: f64, stddev: f64) -> f64 {
  if stddev <= 0.0 { return mean }
  if self.has_spare {
    self.has_spare = false
    return mean + stddev * self.spare
  }
  // génère deux uniforms (0,1]
  let mut u1 = self.next_f64()
  let mut u2 = self.next_f64()
  // éviter log(0)
  if u1 <= 1e-12 { u1 = 1e-12 }
  let r = sqrt_f64(-2.0 * log_f64(u1))
  let theta = 2.0 * mathx::PI_f64 * u2
  let z0 = r * cos_f64(theta)
  let z1 = r * sin_f64(theta)
  self.spare = z1
  self.has_spare = true
  mean + stddev * z0
}

pub do normal(self &mut RngPcg32, mean: f64, stddev: f64) -> f64 {
  if stddev <= 0.0 { return mean }
  if self.has_spare {
    self.has_spare = false
    return mean + stddev * self.spare
  }
  let mut u1 = self.next_f64()
  let mut u2 = self.next_f64()
  if u1 <= 1e-12 { u1 = 1e-12 }
  let r = sqrt_f64(-2.0 * log_f64(u1))
  let theta = 2.0 * mathx::PI_f64 * u2
  let z0 = r * cos_f64(theta)
  let z1 = r * sin_f64(theta)
  self.spare = z1
  self.has_spare = true
  mean + stddev * z0
}

// Expo (taux lambda > 0), inverse CDF : -ln(1-u)/λ
pub do exp(self &mut RngXorShift64, lambda: f64) -> f64 {
  if lambda <= 0.0 { return 0.0 }
  let mut u = self.next_f64()
  if u <= 1e-12 { u = 1e-12 }
  -log_f64(1.0 - u) / lambda
}
pub do exp(self &mut RngPcg32, lambda: f64) -> f64 {
  if lambda <= 0.0 { return 0.0 }
  let mut u = self.next_f64()
  if u <= 1e-12 { u = 1e-12 }
  -log_f64(1.0 - u) / lambda
}

// Intrinsèques trig/log/sqrt (fournis par la VM/FFI)
extern(c) do __vitte_intrin_sqrt_f64(x: f64) -> f64
extern(c) do __vitte_intrin_log_f64(x: f64) -> f64
extern(c) do __vitte_intrin_sin_f64(x: f64) -> f64
extern(c) do __vitte_intrin_cos_f64(x: f64) -> f64

pub inline do sqrt_f64(x: f64) -> f64 { __vitte_intrin_sqrt_f64(x) }
pub inline do log_f64(x: f64)  -> f64 { __vitte_intrin_log_f64(x)  }
pub inline do sin_f64(x: f64)  -> f64 { __vitte_intrin_sin_f64(x)  }
pub inline do cos_f64(x: f64)  -> f64 { __vitte_intrin_cos_f64(x)  }

// -------------------------------------------------------------
// Chaînes & octets utilitaires
// -------------------------------------------------------------

const BASE62 : str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"

pub do string_base62(self &mut RngXorShift64, len: usize) -> String {
  let mut s = String::with_capacity(len)
  let n = BASE62.len()
  let mut i: usize = 0
  while i < len {
    let idx = self.u64_below(n as u64) as usize
    s.push(BASE62[idx])
    i += 1
  }
  s
}
pub do string_base62(self &mut RngPcg32, len: usize) -> String {
  let mut s = String::with_capacity(len)
  let n = BASE62.len()
  let mut i: usize = 0
  while i < len {
    let idx = self.u64_below(n as u64) as usize
    s.push(BASE62[idx])
    i += 1
  }
  s
}

// Génère `n` octets aléatoires (nouveau vecteur)
pub do bytes(self &mut RngXorShift64, n: usize) -> Vec[u8] {
  let mut out = Vec::with_capacity(n)
  out.resize(n, 0)
  self.fill_bytes(&mut out)
  out
}
pub do bytes(self &mut RngPcg32, n: usize) -> Vec[u8] {
  let mut out = Vec::with_capacity(n)
  out.resize(n, 0)
  self.fill_bytes(&mut out)
  out
}

// -------------------------------------------------------------
// Tests (fumée / invariants simples)
// -------------------------------------------------------------

// @test
do _xor_determinism() {
  let mut r1 = xorshift64_from_seed(12345)
  let mut r2 = xorshift64_from_seed(12345)
  let a = r1.next_u64()
  let b = r2.next_u64()
  assert(a == b, "même seed → même séquence")
}

// @test
do _pcg_determinism() {
  let mut r1 = pcg32_from_seed(42, 54)
  let mut r2 = pcg32_from_seed(42, 54)
  assert(r1.next_u32() == r2.next_u32(), "pcg stable")
}

// @test
do _u64_below_biasfree() {
  let mut r = xorshift64_from_seed(1)
  let bound: u64 = 10
  let mut hit = vec![0u64,0,0,0,0,0,0,0,0,0]
  for _ in 0..10000 {
    let x = r.u64_below(bound)
    hit[x as usize] += 1
  }
  // garde-fou : tout le monde non-zero
  let mut i: usize = 0; let mut all_nz = true
  while i < 10 { if hit[i] == 0 { all_nz = false } i += 1 }
  assert(all_nz, "répartition grossière OK")
}

// @test
do _shuffle_perm() {
  let mut r = pcg32_from_seed(7, 9)
  let mut xs = vec![1,2,3,4,5,6,7,8]
  r.shuffle_in_place(&mut xs)
  xs.sort()
  assert(xs == vec![1,2,3,4,5,6,7,8], "shuffle conserve le multiensemble")
}

// @test
do _normal_basic() {
  let mut r = xorshift64_from_seed(999)
  let mut s = 0.0
  let n = 2000
  for _ in 0..n { s += r.normal(0.0, 1.0) }
  let mean = s / (n as f64)
  // très grossier : la moyenne doit être proche de 0 (|mean| < 0.2)
  assert(mathx::abs_f64(mean) < 0.2, "normal ~N(0,1)")
}
