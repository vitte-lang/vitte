# RFC 0025 — Networking and Distributed Systems

## Résumé
Ce RFC définit le support réseau et les primitives pour la création de **systèmes distribués** dans le langage **Vitte**.  
Objectif : fournir des abstractions modernes, performantes et sûres pour écrire des applications client-serveur, des microservices et des protocoles distribués.

## Motivation
Les applications modernes nécessitent :  
- communication réseau efficace (TCP, UDP, QUIC),  
- primitives de haut niveau pour RPC et messaging,  
- support de la tolérance aux pannes et du clustering,  
- sécurité intégrée (TLS, chiffrement).  

Sans cela, Vitte resterait limité aux applications locales.

## Design proposé

### Module `std::net`
- Support TCP/UDP/QUIC.  
- API simple et async :  

```vitte
use std::net

async fn main() {
    let listener = net::TcpListener::bind("0.0.0.0:8080").await
    loop {
        let (socket, addr) = listener.accept().await
        spawn handle_client(socket)
    }
}

async fn handle_client(mut sock: net::TcpStream) {
    sock.write("Hello client!").await
}
```

### Support TLS
- Module `std::net::tls`.  
- Certificats gérés via `vitpm tls`.  

```vitte
use std::net::tls

fn main() {
    let server = tls::Server::new("cert.pem", "key.pem")
    server.listen("0.0.0.0:443")
}
```

### RPC natif
- Module `std::rpc`.  
- Génération automatique de stubs via macros.  

```vitte
@rpc
fn add(a: i32, b: i32) -> i32

async fn main() {
    let client = rpc::connect("localhost:9000").await
    let res = client.add(2, 3).await
    print(res)
}
```

### Messaging distribué
- Canal distribué basé sur `Channel<T>`.  
- Support Kafka-like (`vitmq`).  

```vitte
use std::dist::channel

async fn main() {
    let (tx, rx) = channel::open("topic1")
    tx.send("msg").await
    let m = rx.recv().await
    print(m)
}
```

### Gestion des clusters
- Découverte de services (`std::dist::discovery`).  
- Équilibrage de charge intégré.  
- Reconnexion automatique en cas de panne.  

### Exemple complet : microservice HTTP
```vitte
use std::http

async fn main() {
    http::server("0.0.0.0:8080", |req| async {
        http::Response::ok("Hello Vitte Web!")
    }).await
}
```

### Intégration CI/CD
- Tests de services distribués via `vitpm test net`.  
- Mock réseau pour environnements isolés.  

## Alternatives considérées
- **Libs externes uniquement** : rejeté, car manque d’uniformité.  
- **Support uniquement TCP/UDP** : rejeté, QUIC et TLS indispensables.  
- **Pas de RPC natif** : rejeté, car usage fréquent en distribué.  

## Impact et compatibilité
- Impact fort : ouvre Vitte aux microservices et applications réseau.  
- Compatible avec HTTP/3, gRPC, WebSockets.  
- Introduit une certaine complexité mais adoption facilitée.  

## Références
- [Rust Tokio](https://tokio.rs/)  
- [Go net/http](https://pkg.go.dev/net/http)  
- [gRPC](https://grpc.io/)  
- [QUIC protocol](https://datatracker.ietf.org/wg/quic/documents/)  
