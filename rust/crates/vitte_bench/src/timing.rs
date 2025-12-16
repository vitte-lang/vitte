//! Timing utilities with cross-platform support

#[cfg(target_os = "macos")]
use std::os::raw::c_ulonglong;

/// High-resolution timer for nanosecond-precision measurements
pub struct Timer {
    start: u64,
}

impl Timer {
    pub fn start() -> Self {
        Timer {
            start: Self::now_ns(),
        }
    }

    pub fn elapsed_ns(&self) -> u64 {
        Self::now_ns() - self.start
    }

    #[cfg(target_os = "macos")]
    pub fn now_ns() -> u64 {
        extern "C" {
            fn mach_absolute_time() -> c_ulonglong;
            fn mach_timebase_info(info: *mut mach_timebase_info_t) -> i32;
        }

        #[repr(C)]
        struct mach_timebase_info_t {
            numer: u32,
            denom: u32,
        }

        unsafe {
            let mut timebase = mach_timebase_info_t {
                numer: 0,
                denom: 0,
            };
            let _ = mach_timebase_info(&mut timebase);

            let clock_ticks = mach_absolute_time() as u64;
            (clock_ticks * timebase.numer as u64) / timebase.denom as u64
        }
    }

    #[cfg(target_os = "linux")]
    pub fn now_ns() -> u64 {
        extern "C" {
            pub fn clock_gettime(clk_id: libc::clockid_t, tp: *mut libc::timespec) -> i32;
        }

        unsafe {
            let mut ts: libc::timespec = std::mem::zeroed();
            let _ = clock_gettime(libc::CLOCK_MONOTONIC, &mut ts);
            (ts.tv_sec as u64) * 1_000_000_000 + (ts.tv_nsec as u64)
        }
    }

    #[cfg(target_os = "windows")]
    pub fn now_ns() -> u64 {
        extern "C" {
            fn QueryPerformanceCounter(lpPerformanceCount: *mut i64) -> i32;
            fn QueryPerformanceFrequency(lpFrequency: *mut i64) -> i32;
        }

        unsafe {
            let mut count: i64 = 0;
            let mut freq: i64 = 0;
            let _ = QueryPerformanceCounter(&mut count);
            let _ = QueryPerformanceFrequency(&mut freq);
            ((count as f64 / freq as f64) * 1e9) as u64
        }
    }

    #[cfg(not(any(target_os = "macos", target_os = "linux", target_os = "windows")))]
    pub fn now_ns() -> u64 {
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map(|d| d.as_nanos() as u64)
            .unwrap_or(0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_timer_elapsed() {
        let timer = Timer::start();
        std::thread::sleep(std::time::Duration::from_millis(1));
        let elapsed = timer.elapsed_ns();
        assert!(elapsed >= 1_000_000);
    }
}
