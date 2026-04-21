# Types, contrats et logique de domaine

Cette page traite la logique Vitte comme un mecanisme executable: chaque regle est rattachee a une consequence concrete dans le flux de programme.
La logique Vitte est maximale quand les types encodent les etats et que les procedures encodent les transitions autorisees.

```vit
form Payment {
  amount: int
}

pick PaymentState {
  case Accepted
  case Rejected(code: int)
}
```

Pourquoi cette etape est solide. `Payment` definit la structure d'entree, `PaymentState` definit l'espace de sortie. Le domaine est borne des signatures.

Ce qui se passe a l'execution. Aucune execution directe ici, mais toute procedure future traitera ces deux formes comme contrat compile-time.

```vit
proc decide(p: Payment) -> PaymentState {
  if p.amount > 0 { give Accepted }
  give Rejected(400)
}
```

Pourquoi cette etape est solide. Transition de domaine explicite. Le montant positif mappe sur `Accepted`, sinon `Rejected(400)`. Cette fonction est pure et totalE sur `Payment`.

Ce qui se passe a l'execution. `decide(Payment(50))=Accepted`, `decide(Payment(0))=Rejected(400)`.

```vit
proc code(s: PaymentState) -> int {
  match s {
    case Accepted { give 200 }
    case Rejected(c) { give c }
    otherwise { give 500 }
  }
}
```

Pourquoi cette etape est solide. Projection de l'etat metier vers code technique. La variante porte deja l'information necessaire, donc la conversion reste locale.

Ce qui se passe a l'execution. `code(Accepted)=200`, `code(Rejected(422))=422`.

Ce schema types -> transition -> projection est un pattern Vitte central pour limiter les erreurs d'integration.
