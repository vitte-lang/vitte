# RFC 0047 — Blockchain & Decentralized Applications

## Résumé
Ce RFC propose une intégration native de la **blockchain** et des **applications décentralisées (dApps)** dans le langage **Vitte**.  
Objectif : fournir des primitives sûres, performantes et interopérables pour développer des contrats intelligents, gérer des transactions et bâtir des systèmes distribués résilients.

## Motivation
La blockchain et le Web3 transforment de nombreux domaines :  
- Finance décentralisée (DeFi),  
- Identité numérique et authentification,  
- NFT et économie créative,  
- Supply chain et traçabilité.  

Vitte doit permettre de créer des dApps et smart contracts avec une ergonomie moderne, une sûreté mémoire et une interopérabilité avec Ethereum, Polkadot, Solana, etc.

## Design proposé

### Module `std::chain`
- Gestion des transactions et smart contracts.  
- Support multi-chaînes (Ethereum, Polkadot, Cosmos, Solana).  

```vitte
use std::chain::{Transaction, Contract}

fn main() {
    let tx = Transaction::new("alice", "bob", 10)
    tx.sign("alice_private_key")
    tx.send("ethereum")
}
```

### Contrats intelligents
- Écriture directe de smart contracts en Vitte.  

```vitte
contract Token {
    let balance: Map<Address, u64>

    fn transfer(from: Address, to: Address, amount: u64) {
        if balance[from] >= amount {
            balance[from] -= amount
            balance[to] += amount
        }
    }
}
```

### Stockage décentralisé
- Intégration IPFS et Filecoin.  

```vitte
use std::chain::storage

fn main() {
    let cid = storage::ipfs_put("document.pdf")
    print("CID: {}", cid)
}
```

### Exemple complet : NFT marketplace
```vitte
contract NFTMarket {
    let owners: Map<u64, Address>

    fn mint(id: u64, owner: Address) {
        owners[id] = owner
    }

    fn transfer(id: u64, from: Address, to: Address) {
        if owners[id] == from {
            owners[id] = to
        }
    }
}
```

## Alternatives considérées
- **Langages externes (Solidity, Rust Substrate)** : rejeté, trop fragmentés.  
- **API externes seulement** : rejeté, besoin d’intégration native.  
- **Ignorer blockchain** : rejeté, usage massif prévu.  

## Impact et compatibilité
- Adoption par développeurs Web3 et DeFi.  
- Compatible Ethereum EVM, WASM (Polkadot, Cosmos), Solana BPF.  
- Positionne Vitte comme langage universel pour Web3 + Web2.  

## Références
- [Ethereum Yellow Paper](https://ethereum.github.io/yellowpaper/paper.pdf)  
- [Polkadot Substrate](https://substrate.io/)  
- [IPFS Protocol](https://ipfs.tech/)  
- [W3C Decentralized Identifiers](https://www.w3.org/TR/did-core/)  
