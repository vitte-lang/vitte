# Stdlib Étendue — Guide

- `core` : Option, Result, Iter, Stringx, Vec/Map/Set
- `math` : mathx, rand (XorShift32), decimal
- `sys`  : fs, path, env, process, time
- `net`  : http_client, tcp
- `concurrent` : thread, channel, taskpool
- `formats` : json, csv, ini
- `crypto` : base64, hash, hmac
- `utils` : log, uuid, validate

### Exemples

#### HTTP + JSON
```vitte
use net.http_client as http;
use formats.json as json;

fn main() {
  let body = http.get("https://httpbin.org/json").unwrap();
  let j = json.parse(body);
  print(json.get(j, "slideshow.title").unwrap_or("n/a"));
}
```

#### Fichiers
```vitte
use sys.fs as fs;
fn main() { fs.write_string("hello.txt", "ok"); print(fs.read_to_string("hello.txt").unwrap()); }
```
