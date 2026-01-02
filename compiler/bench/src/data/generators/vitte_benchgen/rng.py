

"""vitte_benchgen.rng

Deterministic RNG utilities for corpus/fixture generation.

Design constraints:
- Outputs must be stable across platforms, Python versions, and runs.
- Avoid using `random.Random` (implementation stable but not a contract we want to depend on).
- Provide:
  - seed parsing
  - stable 64-bit PRNG
  - helpers for sampling, shuffling, and weighted choice

This module is intentionally dependency-free.
"""

from __future__ import annotations

import hashlib
from dataclasses import dataclass
from typing import Iterable, List, Optional, Sequence, Tuple, TypeVar

T = TypeVar("T")


# -----------------------------------------------------------------------------
# Seed utilities
# -----------------------------------------------------------------------------


def seed_from_string(seed: str) -> int:
    """Convert an arbitrary string into a deterministic 64-bit seed."""

    if seed is None:
        seed = ""
    h = hashlib.sha256(seed.encode("utf-8"))
    # first 8 bytes as little-endian u64
    return int.from_bytes(h.digest()[:8], "little", signed=False)


def seed_from_bytes(b: bytes) -> int:
    """Convert bytes into a deterministic 64-bit seed."""

    h = hashlib.sha256(b)
    return int.from_bytes(h.digest()[:8], "little", signed=False)


# -----------------------------------------------------------------------------
# SplitMix64 (seed mixer)
# -----------------------------------------------------------------------------


def _u64(x: int) -> int:
    return x & 0xFFFFFFFFFFFFFFFF


def splitmix64(x: int) -> int:
    """SplitMix64 mix function."""

    z = _u64(x + 0x9E3779B97F4A7C15)
    z = _u64((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9)
    z = _u64((z ^ (z >> 27)) * 0x94D049BB133111EB)
    z = _u64(z ^ (z >> 31))
    return z


# -----------------------------------------------------------------------------
# Xoroshiro128+ (fast stable PRNG)
# -----------------------------------------------------------------------------


def _rotl(x: int, k: int) -> int:
    return _u64((_u64(x) << k) | (_u64(x) >> (64 - k)))


@dataclass
class Rng:
    """Deterministic 128-bit-state RNG."""

    s0: int
    s1: int

    @classmethod
    def from_seed(cls, seed: int | str | bytes) -> "Rng":
        if isinstance(seed, str):
            x = seed_from_string(seed)
        elif isinstance(seed, (bytes, bytearray)):
            x = seed_from_bytes(bytes(seed))
        else:
            x = int(seed)
        # expand to 128-bit state via splitmix
        s0 = splitmix64(_u64(x))
        s1 = splitmix64(_u64(s0))
        # avoid all-zero
        if s0 == 0 and s1 == 0:
            s1 = 1
        return cls(s0=_u64(s0), s1=_u64(s1))

    def next_u64(self) -> int:
        """Return next u64."""

        s0 = self.s0
        s1 = self.s1
        out = _u64(s0 + s1)

        s1 ^= s0
        self.s0 = _u64(_rotl(s0, 55) ^ s1 ^ (s1 << 14))
        self.s1 = _u64(_rotl(s1, 36))
        return out

    def next_u32(self) -> int:
        return (self.next_u64() >> 32) & 0xFFFFFFFF

    def next_i32(self) -> int:
        v = self.next_u32()
        # map to signed
        return v - 0x100000000 if v & 0x80000000 else v

    def random(self) -> float:
        """Return float in [0,1). Uses 53 bits."""

        # 53-bit precision
        v = self.next_u64() >> 11
        return v / float(1 << 53)

    def randint(self, lo: int, hi: int) -> int:
        """Uniform integer in [lo, hi] inclusive."""

        if lo > hi:
            lo, hi = hi, lo
        span = hi - lo + 1
        # rejection sampling to avoid modulo bias
        if span <= 0:
            # overflow span; fallback to u64 range mapping
            return lo + (self.next_u64() % _u64(hi - lo + 1))

        limit = (1 << 64) - ((1 << 64) % span)
        while True:
            r = self.next_u64()
            if r < limit:
                return lo + (r % span)

    def choice(self, seq: Sequence[T]) -> T:
        if not seq:
            raise ValueError("choice from empty sequence")
        idx = self.randint(0, len(seq) - 1)
        return seq[idx]

    def shuffle(self, items: List[T]) -> None:
        """In-place Fisher-Yates."""

        n = len(items)
        for i in range(n - 1, 0, -1):
            j = self.randint(0, i)
            items[i], items[j] = items[j], items[i]

    def sample(self, seq: Sequence[T], k: int) -> List[T]:
        """Sample k unique elements without replacement."""

        if k < 0:
            raise ValueError("k must be >= 0")
        n = len(seq)
        if k > n:
            raise ValueError("k larger than population")
        if k == 0:
            return []
        if k == n:
            return list(seq)

        # For small k, do selection by partial shuffle of indices
        idxs = list(range(n))
        for i in range(k):
            j = self.randint(i, n - 1)
            idxs[i], idxs[j] = idxs[j], idxs[i]
        return [seq[i] for i in idxs[:k]]

    def weighted_choice(self, items: Sequence[T], weights: Sequence[float]) -> T:
        if len(items) != len(weights):
            raise ValueError("items/weights length mismatch")
        if not items:
            raise ValueError("weighted_choice from empty sequence")

        total = 0.0
        for w in weights:
            if w < 0.0:
                raise ValueError("negative weight")
            total += w

        if total <= 0.0:
            # all zero weights => uniform
            return self.choice(items)

        r = self.random() * total
        acc = 0.0
        for it, w in zip(items, weights):
            acc += w
            if r < acc:
                return it
        return items[-1]


# -----------------------------------------------------------------------------
# Utility: stable shuffling by key
# -----------------------------------------------------------------------------


def stable_shuffle_by_key(seed: int | str | bytes, paths: Sequence[str]) -> List[str]:
    """Return paths in a deterministic pseudo-random order.

    This is often preferable to RNG-based shuffling because it remains stable
    even when paths are added/removed: each path's rank is independent.

    Rank = sha256(seed + "\n" + path).
    """

    if isinstance(seed, str):
        seed_b = seed.encode("utf-8")
    elif isinstance(seed, (bytes, bytearray)):
        seed_b = bytes(seed)
    else:
        seed_b = str(int(seed)).encode("utf-8")

    def rank(p: str) -> bytes:
        h = hashlib.sha256()
        h.update(seed_b)
        h.update(b"\n")
        h.update(p.encode("utf-8"))
        return h.digest()

    return sorted(list(paths), key=rank)


__all__ = [
    "Rng",
    "seed_from_string",
    "seed_from_bytes",
    "splitmix64",
    "stable_shuffle_by_key",
]