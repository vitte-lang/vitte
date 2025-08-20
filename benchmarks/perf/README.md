# Profilage & Perf

## Linux (perf + FlameGraph)
```bash
sudo apt-get install -y linux-tools-common linux-tools-generic
cargo bench --bench tokenizer -- --profile-time 5
# Exemple de perf record sur un bench:
perf record -F 99 -g -- target/release/deps/tokenizer-*
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
xdg-open flame.svg
```

## macOS
- Instruments: Time Profiler
- DTrace: `sudo dtrace -n 'profile-997 /pid == $target/ {{ @[ustack()] = count(); }}' -p <PID>`
