//! Statistical analysis utilities

/// Simple statistics computation
#[derive(Debug, Clone)]
pub struct Stats {
    samples: Vec<f64>,
}

impl Stats {
    pub fn new(samples: Vec<f64>) -> Self {
        let mut sorted = samples.clone();
        sorted.sort_by(|a, b| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal));
        Stats { samples: sorted }
    }

    pub fn mean(&self) -> f64 {
        if self.samples.is_empty() {
            return 0.0;
        }

        // Kahan summation for numerical stability
        let mut sum = 0.0;
        let mut c = 0.0;

        for &x in &self.samples {
            let y = x - c;
            let t = sum + y;
            c = (t - sum) - y;
            sum = t;
        }

        sum / self.samples.len() as f64
    }

    pub fn median(&self) -> f64 {
        if self.samples.is_empty() {
            return 0.0;
        }
        let mid = self.samples.len() / 2;
        if self.samples.len() % 2 == 0 {
            (self.samples[mid - 1] + self.samples[mid]) / 2.0
        } else {
            self.samples[mid]
        }
    }

    pub fn percentile(&self, p: f64) -> f64 {
        if self.samples.is_empty() {
            return 0.0;
        }
        let idx = ((p / 100.0) * self.samples.len() as f64).ceil() as usize;
        self.samples[idx.saturating_sub(1).min(self.samples.len() - 1)]
    }

    pub fn p95(&self) -> f64 {
        self.percentile(95.0)
    }

    pub fn p50(&self) -> f64 {
        self.percentile(50.0)
    }

    pub fn min(&self) -> f64 {
        self.samples.first().copied().unwrap_or(0.0)
    }

    pub fn max(&self) -> f64 {
        self.samples.last().copied().unwrap_or(0.0)
    }

    pub fn stddev(&self) -> f64 {
        let mean = self.mean();
        let variance = self
            .samples
            .iter()
            .map(|x| (x - mean).powi(2))
            .sum::<f64>()
            / self.samples.len() as f64;
        variance.sqrt()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_stats_basic() {
        let data = vec![1.0, 2.0, 3.0, 4.0, 5.0];
        let stats = Stats::new(data);
        assert_eq!(stats.mean(), 3.0);
        assert_eq!(stats.median(), 3.0);
        assert_eq!(stats.min(), 1.0);
        assert_eq!(stats.max(), 5.0);
    }

    #[test]
    fn test_stats_percentile() {
        let data = (1..=100).map(|i| i as f64).collect::<Vec<_>>();
        let stats = Stats::new(data);
        assert!(stats.p95() >= 95.0);
    }
}
