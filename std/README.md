# Vitte Stdlib — Ultimate

Librairie standard étendue pour **Vitte**.
- Organisation modulaire (core, math, sys, net, concurrent, formats, crypto, utils)
- API stables, docs inline, tests d’usage

## Utilisation
```vitte
use std.core.option as Option;
use std.net.http as http;

fn main() {
  let body = http.get("https://example.com").unwrap();
  print(body);
}
```
