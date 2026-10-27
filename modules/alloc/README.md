# Vitte alloc

`alloc` consomme les contrats mémoire de `core` et fournit des interfaces
d'allocation utilisables sans import C depuis les modules Vitte.

## Allocateurs disponibles

- `allocator::system` réserve via `core::intrinsics::memory::native_allocate`,
  aligne le pointeur et conserve l'adresse de base pour la libération. La
  libération et la réallocation exigent un pointeur système retourné par ce
  module et le layout correspondant.
- `allocator::zeroed` met les octets à zéro avant de rendre le bloc.
- `allocator::null_alloc` accepte les layouts de taille nulle et échoue pour
  toute réservation non vide.
- `allocator::custom` fournit un allocateur de pile sur un tampon de l'appelant;
  la libération ne réussit que pour le bloc le plus récent. `custom_reset`
  invalide toutes les réservations précédentes.
- `allocator::fallback`, `allocator::composite`, `allocator::limiting`,
  `allocator::counting` et `allocator::tracking` adaptent l'interface uniforme
  `allocator::Allocator`. Les compteurs et limites supposent des paires
  allocation/libération valides.
- `allocator::global` choisit actuellement l'allocateur système; il n'existe pas
  encore de remplacement global mutable.

Tous les allocateurs renvoient `allocator::AllocationResult`. Une réservation
de taille nulle réussie renvoie un pointeur nul et ne requiert aucune libération.
Les layouts proviennent de `core::mem::layout::Layout`; les façades
`layout::*` et `memory::*` de ce paquet délèguent les calculs et opérations aux
fonctions de `core`.

## Concurrence et limites du runtime

Les adaptateurs atomiques et verrous déjà présents dans `concurrency` délèguent
à `core::sync`. Le compilateur bootstrap ne valide pas encore les appels
génériques des intrinsics atomiques de `core`, donc ces adaptateurs ne sont pas
encore utilisables sur la voie native.

`allocator::thread_local` contient un état explicite par instance; il ne crée
pas de stockage local au thread. Les modules `concurrency::epoch`,
`concurrency::hazard_pointer`, `concurrency::thread_cache` et
`concurrency::concurrent_pool` restent en attente des atomiques abaissés par le
backend et du TLS du runtime. `memory::virtual_memory` attend des primitives
cibles de réservation et de libération de pages virtuelles. Aucun de ces
contrats n'est émulé avec un verrou non atomique.

Les modules `arena`, `containers`, `heap`, `pool`, `gc`, `safety`, `platform` et
les diagnostics d'allocation restent à compléter; les fichiers encore vides
dans ces répertoires ne constituent pas une API implémentée.
