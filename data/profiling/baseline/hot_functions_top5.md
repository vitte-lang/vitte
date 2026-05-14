# Top 5 Hot Functions (CPU/Memory)

1. `executor_run` - 21.8% CPU
2. `future_await` - 17.4% CPU
3. `ffi_call` - 13.2% CPU
4. `serialize_to_json` - 11.6% CPU
5. `threadpool_submit` - 9.1% CPU

Memory hotspots:
- `serialize_to_json`: transient buffer pressure
- `future_all`: aggregate result allocation bursts
