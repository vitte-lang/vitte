#![deny(unsafe_code)]
#![forbid(clippy::unwrap_used)]
//! vitte-collections — structures de données pour Vitte
//!
//! Modules principaux :
//! - `arena`  : allocation indexée stable (handles) avec itération
//! - `slab`   : allocation d’IDs denses type slab (insert/remove O(1))
//! - `bitset` : ensembles de bits compacts (ops en mots 64)
//! - `smap`   : SmallVecMap — micro‐map linéaire optimisée pour petits N
//! - `igraph` : graphe orienté à listes d’adjacence + BFS/DFS/Dijkstra
//!
//! Tous les modules sont `#[no_std]` compatibles hors quelques helpers test.
//!
//! Exemple rapide :
//! ```
//! use vitte_collections::arena::Arena;
//! let mut a = Arena::<String>::new();
//! let id = a.insert("ok".to_string());
//! assert_eq!(a[id].as_str(), "ok");
//! ```

extern crate alloc;

pub mod arena {
    use alloc::{vec::Vec, fmt, string::String};
    use core::{ops::{Index, IndexMut}, marker::PhantomData};

    /// Handle indexé type‐safe.
    #[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
    pub struct Id<T> { idx: u32, _p: PhantomData<T> }
    impl<T> Id<T> {
        #[inline] pub const fn from_raw(idx: u32) -> Self { Self { idx, _p: PhantomData } }
        #[inline] pub const fn index(self) -> usize { self.idx as usize }
    }
    impl<T> fmt::Debug for Id<T> {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "Id({})", self.idx) }
    }

    /// Arena simple : indices stables, pas de free individuelle.
    #[derive(Clone, Default)]
    pub struct Arena<T> {
        data: Vec<T>,
    }
    impl<T> Arena<T> {
        #[inline] pub const fn new() -> Self { Self { data: Vec::new() } }
        #[inline] pub fn with_capacity(c: usize) -> Self { Self { data: Vec::with_capacity(c) } }
        #[inline] pub fn capacity(&self) -> usize { self.data.capacity() }
        #[inline] pub fn len(&self) -> usize { self.data.len() }
        #[inline] pub fn is_empty(&self) -> bool { self.data.is_empty() }

        #[inline]
        pub fn insert(&mut self, value: T) -> Id<T> {
            let id = Id::from_raw(self.data.len() as u32);
            self.data.push(value);
            id
        }

        /// Pousse en batch, retourne les Ids dans l’ordre.
        pub fn extend<I: IntoIterator<Item = T>>(&mut self, it: I) -> Vec<Id<T>> {
            let mut out = Vec::new();
            for v in it {
                out.push(self.insert(v));
            }
            out
        }

        #[inline] pub fn get(&self, id: Id<T>) -> Option<&T> { self.data.get(id.index()) }
        #[inline] pub fn get_mut(&mut self, id: Id<T>) -> Option<&mut T> { self.data.get_mut(id.index()) }
        #[inline] pub fn iter(&self) -> impl Iterator<Item = (Id<T>, &T)> {
            self.data.iter().enumerate().map(|(i, v)| (Id::from_raw(i as u32), v))
        }
        #[inline] pub fn iter_mut(&mut self) -> impl Iterator<Item = (Id<T>, &mut T)> {
            self.data.iter_mut().enumerate().map(|(i, v)| (Id::from_raw(i as u32), v))
        }

        /// Échange l’élément `a` et `b`.
        pub fn swap(&mut self, a: Id<T>, b: Id<T>) {
            self.data.swap(a.index(), b.index());
        }

        /// Convertit en `Vec<T>` (invalide les Ids).
        #[inline] pub fn into_inner(self) -> Vec<T> { self.data }
    }
    impl<T> Index<Id<T>> for Arena<T> {
        type Output = T;
        #[inline] fn index(&self, index: Id<T>) -> &Self::Output { &self.data[index.index()] }
    }
    impl<T> IndexMut<Id<T>> for Arena<T> {
        #[inline] fn index_mut(&mut self, index: Id<T>) -> &mut Self::Output { &mut self.data[index.index()] }
    }
}

pub mod slab {
    use alloc::vec::Vec;

    /// Slot d’un slab : libre ou occupé.
    #[derive(Clone, Copy)]
    enum Slot {
        Free(Option<u32>), // next free
        Filled,
    }

    /// Slab denses avec récupération O(1).
    pub struct Slab<T> {
        slots: Vec<Slot>,
        data: Vec<Option<T>>,
        head_free: Option<u32>,
        len: usize,
    }

    impl<T> Default for Slab<T> { fn default() -> Self { Self::new() } }
    impl<T> Slab<T> {
        pub fn new() -> Self {
            Self { slots: Vec::new(), data: Vec::new(), head_free: None, len: 0 }
        }

        pub fn insert(&mut self, value: T) -> u32 {
            if let Some(i) = self.head_free {
                // reuse
                let next = match self.slots[i as usize] {
                    Slot::Free(n) => n,
                    Slot::Filled => unreachable!(),
                };
                self.slots[i as usize] = Slot::Filled;
                self.data[i as usize] = Some(value);
                self.head_free = next;
                self.len += 1;
                i
            } else {
                let i = self.slots.len() as u32;
                self.slots.push(Slot::Filled);
                self.data.push(Some(value));
                self.len += 1;
                i
            }
        }

        pub fn remove(&mut self, key: u32) -> Option<T> {
            let idx = key as usize;
            if idx >= self.data.len() { return None; }
            if matches!(self.slots[idx], Slot::Filled) {
                self.slots[idx] = Slot::Free(self.head_free);
                self.head_free = Some(key);
                self.len -= 1;
                return self.data[idx].take();
            }
            None
        }

        #[inline] pub fn get(&self, key: u32) -> Option<&T> {
            self.data.get(key as usize)?.as_ref()
        }
        #[inline] pub fn get_mut(&mut self, key: u32) -> Option<&mut T> {
            self.data.get_mut(key as usize)?.as_mut()
        }
        #[inline] pub fn len(&self) -> usize { self.len }
        #[inline] pub fn is_empty(&self) -> bool { self.len == 0 }

        pub fn iter(&self) -> impl Iterator<Item = (u32, &T)> {
            self.data.iter().enumerate().filter_map(|(i, v)| v.as_ref().map(|r| (i as u32, r)))
        }
    }
}

pub mod bitset {
    use alloc::vec::Vec;

    #[derive(Clone, Debug, Default, PartialEq, Eq)]
    pub struct BitSet {
        words: Vec<u64>,
        len: usize, // nombre de bits logiques
    }

    impl BitSet {
        #[inline] pub const fn new() -> Self { Self { words: Vec::new(), len: 0 } }
        pub fn with_len(bits: usize) -> Self {
            let wc = (bits + 63) / 64;
            let mut words = Vec::with_capacity(wc);
            words.resize(wc, 0);
            Self { words, len: bits }
        }
        #[inline] pub fn len(&self) -> usize { self.len }
        #[inline] pub fn is_empty(&self) -> bool { self.len == 0 }

        fn ensure(&mut self, bit: usize) {
            if bit >= self.len { self.resize(bit + 1); }
        }
        pub fn resize(&mut self, new_len: usize) {
            self.len = new_len;
            let need = (new_len + 63) / 64;
            if self.words.len() < need {
                self.words.resize(need, 0);
            }
            if self.words.len() > need {
                self.words.truncate(need);
            }
            if new_len % 64 != 0 {
                let last = self.words.last_mut().unwrap();
                let valid = new_len as u64 % 64;
                if valid != 0 {
                    let mask = (1u64 << valid) - 1;
                    *last &= mask;
                }
            }
        }

        #[inline] pub fn set(&mut self, bit: usize) {
            self.ensure(bit);
            let (w, b) = (bit / 64, bit % 64);
            self.words[w] |= 1u64 << b;
        }
        #[inline] pub fn reset(&mut self, bit: usize) {
            if bit >= self.len { return; }
            let (w, b) = (bit / 64, bit % 64);
            self.words[w] &= !(1u64 << b);
        }
        #[inline] pub fn toggle(&mut self, bit: usize) {
            self.ensure(bit);
            let (w, b) = (bit / 64, bit % 64);
            self.words[w] ^= 1u64 << b;
        }
        #[inline] pub fn test(&self, bit: usize) -> bool {
            if bit >= self.len { return false; }
            let (w, b) = (bit / 64, bit % 64);
            (self.words[w] >> b) & 1 == 1
        }

        pub fn clear_all(&mut self) {
            for w in &mut self.words { *w = 0; }
        }

        /// ET logique in-place avec autre `BitSet` (dimension min).
        pub fn and_inplace(&mut self, other: &BitSet) {
            let n = self.words.len().min(other.words.len());
            for i in 0..n { self.words[i] &= other.words[i]; }
            for i in n..self.words.len() { self.words[i] = 0; }
        }
        /// OU logique in-place.
        pub fn or_inplace(&mut self, other: &BitSet) {
            if other.words.len() > self.words.len() {
                self.words.resize(other.words.len(), 0);
            }
            for i in 0..other.words.len() { self.words[i] |= other.words[i]; }
        }

        pub fn ones(&self) -> impl Iterator<Item = usize> + '_ {
            self.words.iter().enumerate().flat_map(|(wi, &mut w)| {
                let mut bits = w;
                core::iter::from_fn(move || {
                    if bits == 0 { return None; }
                    let t = bits.trailing_zeros() as usize;
                    bits &= bits - 1;
                    Some(wi * 64 + t)
                })
            }).filter(move |&b| b < self.len)
        }
    }
}

pub mod smap {
    use alloc::{vec::Vec, borrow::Borrow};
    use core::fmt::Debug;

    /// Map linéaire stockée dans un `SmallVec` (ici simulée avec `Vec` générique + const N).
    /// Optimale pour N petit (nombre d’entrées ≤ N).
    #[derive(Clone)]
    pub struct SmallVecMap<K, V, const N: usize> {
        items: Vec<(K, V)>,
    }

    impl<K: PartialEq, V, const N: usize> Default for SmallVecMap<K, V, N> {
        fn default() -> Self { Self { items: Vec::with_capacity(N) } }
    }

    impl<K: PartialEq + Debug, V: Debug, const N: usize> core::fmt::Debug for SmallVecMap<K, V, N> {
        fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
            f.debug_map().entries(self.items.iter().map(|(k,v)|(k,v))).finish()
        }
    }

    impl<K: PartialEq, V, const N: usize> SmallVecMap<K, V, N> {
        #[inline] pub fn new() -> Self { Self::default() }
        #[inline] pub fn len(&self) -> usize { self.items.len() }
        #[inline] pub fn is_empty(&self) -> bool { self.items.is_empty() }

        pub fn insert(&mut self, k: K, v: V) -> Option<V> {
            if let Some(i) = self.items.iter().position(|(kk, _)| *kk == k) {
                Some(core::mem::replace(&mut self.items[i].1, v))
            } else {
                self.items.push((k, v));
                None
            }
        }

        pub fn get<Q>(&self, k: &Q) -> Option<&V>
        where
            K: Borrow<Q>,
            Q: PartialEq + ?Sized,
        {
            self.items.iter().find(|(kk, _)| kk.borrow() == k).map(|(_, v)| v)
        }

        pub fn get_mut<Q>(&mut self, k: &Q) -> Option<&mut V>
        where
            K: Borrow<Q>,
            Q: PartialEq + ?Sized,
        {
            self.items.iter_mut().find(|(kk, _)| kk.borrow() == k).map(|(_, v)| v)
        }

        pub fn remove<Q>(&mut self, k: &Q) -> Option<V>
        where
            K: Borrow<Q>,
            Q: PartialEq + ?Sized,
        {
            if let Some(i) = self.items.iter().position(|(kk, _)| kk.borrow() == k) {
                Some(self.items.swap_remove(i).1)
            } else { None }
        }

        pub fn iter(&self) -> impl Iterator<Item = (&K, &V)> { self.items.iter().map(|(k,v)| (k,v)) }
        pub fn iter_mut(&mut self) -> impl Iterator<Item = (&K, &mut V)> { self.items.iter_mut().map(|(k,v)| (k,v)) }
        pub fn into_vec(self) -> Vec<(K,V)> { self.items }
    }
}

pub mod igraph {
    use alloc::vec::Vec;
    use core::cmp::Ordering;
    use crate::bitset::BitSet;

    /// Identifiant sommet.
    pub type NodeId = u32;

    /// Arête `to` + coût optionnel.
    #[derive(Clone, Copy, Debug)]
    pub struct Edge<E> {
        pub to: NodeId,
        pub weight: E,
    }

    /// Graphe orienté avec stockage contigu.
    #[derive(Clone, Debug)]
    pub struct Graph<N, E> {
        pub nodes: Vec<N>,
        pub adj: Vec<Vec<Edge<E>>>, // adjacency list
    }

    impl<N, E: Copy + Default> Graph<N, E> {
        pub fn new() -> Self { Self { nodes: Vec::new(), adj: Vec::new() } }
        pub fn with_capacity(n: usize) -> Self {
            Self { nodes: Vec::with_capacity(n), adj: (0..n).map(|_| Vec::new()).collect() }
        }

        pub fn add_node(&mut self, data: N) -> NodeId {
            let id = self.nodes.len() as NodeId;
            self.nodes.push(data);
            self.adj.push(Vec::new());
            id
        }

        pub fn add_edge(&mut self, from: NodeId, to: NodeId, weight: E) {
            self.adj[from as usize].push(Edge { to, weight });
        }

        /// BFS depuis `src`, retourne ordre de visite.
        pub fn bfs(&self, src: NodeId) -> Vec<NodeId> {
            let n = self.nodes.len();
            let mut seen = BitSet::with_len(n);
            let mut q = alloc::collections::VecDeque::new();
            let mut out = Vec::new();

            seen.set(src as usize);
            q.push_back(src);

            while let Some(u) = q.pop_front() {
                out.push(u);
                for e in &self.adj[u as usize] {
                    if !seen.test(e.to as usize) {
                        seen.set(e.to as usize);
                        q.push_back(e.to);
                    }
                }
            }
            out
        }

        /// DFS itératif.
        pub fn dfs(&self, src: NodeId) -> Vec<NodeId> {
            let n = self.nodes.len();
            let mut seen = BitSet::with_len(n);
            let mut st = Vec::new();
            let mut out = Vec::new();

            st.push(src);
            while let Some(u) = st.pop() {
                if seen.test(u as usize) { continue; }
                seen.set(u as usize);
                out.push(u);
                // push voisins en ordre inverse pour comportement proche récursif
                for e in self.adj[u as usize].iter().rev() {
                    if !seen.test(e.to as usize) {
                        st.push(e.to);
                    }
                }
            }
            out
        }
    }

    /// Dijkstra générique avec poids `W: Copy + Ord + num_traits::Zero + Add<Output=W>`.
    pub mod dijkstra {
        use super::*;
        use core::ops::Add;

        #[derive(Clone, Copy, Debug, PartialEq, Eq)]
        pub struct Dist<W> { pub prev: Option<NodeId>, pub cost: W }

        /// File prioritaire binaire simple.
        struct BinHeap<K, V> {
            data: Vec<(K, V)>,
        }
        impl<K: Ord, V> BinHeap<K, V> {
            fn new() -> Self { Self { data: Vec::new() } }
            fn push(&mut self, k: K, v: V) {
                self.data.push((k, v));
                let mut i = self.data.len() - 1;
                while i > 0 {
                    let p = (i - 1) / 2;
                    if self.data[p].0 <= self.data[i].0 { break; }
                    self.data.swap(p, i);
                    i = p;
                }
            }
            fn pop(&mut self) -> Option<(K, V)> {
                if self.data.is_empty() { return None; }
                let last = self.data.pop().unwrap();
                if self.data.is_empty() { return Some(last); }
                let mut out = core::mem::replace(&mut self.data[0], last);
                // descente
                let mut i = 0;
                loop {
                    let l = 2 * i + 1;
                    let r = 2 * i + 2;
                    if l >= self.data.len() { break; }
                    let mut m = l;
                    if r < self.data.len() && self.data[r].0 < self.data[l].0 { m = r; }
                    if self.data[i].0 <= self.data[m].0 { break; }
                    self.data.swap(i, m);
                    i = m;
                }
                Some(out)
            }
            fn is_empty(&self) -> bool { self.data.is_empty() }
        }

        /// Calcule les distances depuis `src`.  
        /// `W` doit supporter zéro et addition. On demande les callbacks pour `zero` et `add`.
        pub fn shortest_paths<N, W, FAdd, FZero>(
            g: &Graph<N, W>,
            src: NodeId,
            mut add: FAdd,
            mut zero: FZero,
        ) -> Vec<Option<Dist<W>>>
        where
            W: Copy + Ord,
            FAdd: FnMut(W, W) -> W,
            FZero: FnMut() -> W,
        {
            let n = g.nodes.len();
            let mut dist: Vec<Option<Dist<W>>> = (0..n).map(|_| None).collect();
            let mut heap = BinHeap::<W, NodeId>::new();

            dist[src as usize] = Some(Dist { prev: None, cost: zero() });
            heap.push(zero(), src);

            while let Some((du, u)) = heap.pop() {
                if let Some(Dist{ cost, .. }) = dist[u as usize] {
                    if du > cost { continue; }
                }
                for e in &g.adj[u as usize] {
                    let cand = add(du, e.weight);
                    let dv = dist[e.to as usize].map(|d| d.cost);
                    let better = match dv { None => true, Some(old) => cand < old };
                    if better {
                        dist[e.to as usize] = Some(Dist { prev: Some(u), cost: cand });
                        heap.push(cand, e.to);
                    }
                }
            }
            dist
        }

        /// Reconstruit le chemin `src -> dst` en utilisant `prev`.
        pub fn reconstruct_path<W>(info: &[Option<Dist<W>>], dst: NodeId) -> Option<alloc::vec::Vec<NodeId>> {
            let mut cur = dst;
            let mut out = alloc::vec::Vec::new();
            loop {
                let Some(d) = info.get(cur as usize).and_then(|x| *x) else { return None; };
                out.push(cur);
                match d.prev {
                    None => break,
                    Some(p) => cur = p,
                }
            }
            out.reverse();
            Some(out)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use igraph::dijkstra;

    #[test]
    fn arena_basic() {
        let mut a = arena::Arena::<i32>::new();
        let id0 = a.insert(10);
        let id1 = a.insert(20);
        assert_eq!(a[id0], 10);
        assert_eq!(a[id1], 20);
        *a.get_mut(id1).unwrap() = 21;
        assert_eq!(a[id1], 21);
        let all: alloc::vec::Vec<_> = a.iter().map(|(_, v)| *v).collect();
        assert_eq!(all, vec![10, 21]);
    }

    #[test]
    fn slab_insert_remove() {
        let mut s = slab::Slab::new();
        let a = s.insert("a");
        let b = s.insert("b");
        assert_eq!(s.get(a), Some(&"a"));
        assert_eq!(s.get(b), Some(&"b"));
        assert_eq!(s.remove(a), Some("a"));
        let c = s.insert("c");
        // réutilise le slot libre
        assert_eq!(a, c);
    }

    #[test]
    fn bitset_ops() {
        let mut bs = bitset::BitSet::with_len(10);
        bs.set(1); bs.set(3); bs.set(9);
        assert!(bs.test(3));
        bs.reset(3);
        assert!(!bs.test(3));
        let ones: alloc::vec::Vec<_> = bs.ones().collect();
        assert_eq!(ones, vec![1, 9]);
    }

    #[test]
    fn smap_small() {
        let mut m: smap::SmallVecMap<&str, i32, 8> = smap::SmallVecMap::new();
        assert!(m.insert("x", 1).is_none());
        assert_eq!(m.insert("x", 2), Some(1));
        assert_eq!(m.get(&"x"), Some(&2));
        assert_eq!(m.remove(&"x"), Some(2));
        assert!(m.get(&"x").is_none());
    }

    #[test]
    fn graph_bfs_dfs() {
        let mut g = igraph::Graph::<(), i32>::new();
        let a = g.add_node(());
        let b = g.add_node(());
        let c = g.add_node(());
        let d = g.add_node(());
        g.add_edge(a, b, 1);
        g.add_edge(a, c, 1);
        g.add_edge(b, d, 1);
        g.add_edge(c, d, 1);

        let bfs = g.bfs(a);
        let dfs = g.dfs(a);
        assert_eq!(bfs[0], a);
        assert_eq!(dfs[0], a);
        assert!(bfs.contains(&d) && dfs.contains(&d));
    }

    #[test]
    fn graph_dijkstra() {
        let mut g = igraph::Graph::<(), u32>::new();
        let s = g.add_node(());
        let n1 = g.add_node(());
        let n2 = g.add_node(());
        let t = g.add_node(());
        g.add_edge(s, n1, 1);
        g.add_edge(s, n2, 5);
        g.add_edge(n1, n2, 1);
        g.add_edge(n2, t, 1);

        let dist = dijkstra::shortest_paths(&g, s, |a,b| a+b, || 0u32);
        let path = dijkstra::reconstruct_path(&dist, t).unwrap();
        // s -> n1 -> n2 -> t  : coût 1 + 1 + 1 = 3
        assert_eq!(path, vec![s, n1, n2, t]);
        assert_eq!(dist[t as usize].as_ref().unwrap().cost, 3);
    }
}