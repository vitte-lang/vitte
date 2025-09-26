# RFC 0028 — Blockchain and Smart Contracts

## Résumé
Ce RFC définit le support de la **blockchain** et des **smart contracts** dans le langage **Vitte**.  
Objectif : permettre aux développeurs d’écrire des contrats intelligents sûrs et efficaces, exécutables sur différentes blockchains, tout en restant compatibles avec l’écosystème Vitte.

## Motivation
Les blockchains sont devenues une plateforme incontournable pour :  
- la finance décentralisée (DeFi),  
- les NFT et métaverses,  
- la gestion sécurisée des identités et transactions,  
- les systèmes distribués de confiance.  

Un langage moderne comme Vitte doit proposer un support natif pour ces usages.

## Design proposé

### Module `std::chain`
- Primitives blockchain de base :  
  - `block::id()`  
  - `tx::hash()`  
  - `wallet::balance()`  

Exemple :  
```vitte
use std::chain

fn main() {
    let bal = chain::wallet::balance("0x1234abcd")
    print("Balance: {}", bal)
}
```

### Contrats intelligents
- Syntaxe dédiée avec annotation `@contract`.  
- Exemple minimal :  

```vitte
@contract
fn transfer(from: Address, to: Address, amount: u64) -> bool {
    if chain::wallet::balance(from) >= amount {
        chain::tx::transfer(from, to, amount)
        true
    } else {
        false
    }
}
```

### Compilation et déploiement
- `vitpm build --target chain-evm` → génère bytecode EVM.  
- `vitpm build --target chain-solana` → génère bytecode Solana.  
- Déploiement :  
```sh
vitpm deploy --network testnet contract.vitte
```

### Sécurité
- Vérification formelle possible (`vitpm verify`).  
- Système d’ownership appliqué aux contrats pour éviter les fuites et reentrancy attacks.  

### Intégration DeFi et NFT
- Module `std::chain::token`.  
- Exemple ERC20-like :  

```vitte
@contract
fn mint(to: Address, amount: u64) {
    token::increase_supply(to, amount)
}
```

### Exemple complet : NFT minimal
```vitte
@contract
struct NFT {
    id: u64,
    owner: Address
}

fn mint(owner: Address) -> NFT {
    NFT { id: chain::block::id(), owner }
}
```

### Outils CLI
- `vitpm chain status` → état du réseau.  
- `vitpm chain call` → exécution locale.  
- `vitpm chain test` → tests sur VM simulée.  

## Alternatives considérées
- **Pas de support blockchain** : rejeté, adoption limitée.  
- **Bindings externes seulement** : rejeté, manque de sûreté et ergonomie.  
- **Support d’une seule blockchain (EVM-only)** : rejeté, besoin multi-chain.  

## Impact et compatibilité
- Impact majeur : ouvre Vitte au Web3 et à la DeFi.  
- Compatible avec Ethereum, Solana, Polkadot, Cosmos.  
- Introduit une complexité en tooling mais essentielle pour adoption.  

## Références
- [Ethereum](https://ethereum.org/)  
- [Solana](https://solana.com/)  
- [Polkadot](https://polkadot.network/)  
- [Cosmos SDK](https://cosmos.network/)  
- [Rust Ink! (substrate contracts)](https://use.ink/)  
