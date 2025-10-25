# Traductions françaises
status.ok = OK
status.info = INFO
status.warn = AVERT

doctor.status.ok = [OK]
doctor.status.warn = [AVERT]
doctor.status.error = [ERREUR]

doctor.suggestion.inline = suggestion : {text}
doctor.suggestions.title = Suggestions globales :
doctor.suggestions.item = {index}. {text}

doctor.report.header = Vitte Doctor — espace de travail : {workspace} ({errors_label}, {warnings_label})

doctor.count.errors.zero = 0 erreur
doctor.count.errors.one = 1 erreur
doctor.count.errors.other = {count} erreurs

doctor.count.warnings.zero = 0 avertissement
doctor.count.warnings.one = 1 avertissement
doctor.count.warnings.other = {count} avertissements

doctor.check.rustc.name = toolchain rustc
doctor.check.rustc.not_found = rustc introuvable ({error})
doctor.check.rustc.recommend.install = Installez Rust via https://rustup.rs et ajoutez-le à votre PATH.
doctor.check.rustc.exec_failed = `rustc --version` a échoué (code {code})
doctor.check.rustc.recommend.verify = Vérifiez votre installation Rust ou réinstallez rustup.
doctor.check.rustc.outdated = version détectée {version} (minimum recommandé {minimum})
doctor.check.rustc.recommend.update = Mettez à jour Rust via `rustup update`.
doctor.check.rustc.ok = version ok ({version})
doctor.check.rustc.parse_error = sortie inattendue : {output}

doctor.check.cargo.name = binaire cargo
doctor.check.cargo.ok = {version}
doctor.check.cargo.exec_failed = `cargo --version` a retourné le code {code}
doctor.check.cargo.recommend.verify = Vérifiez votre installation Rust (cargo est fourni par rustup).
doctor.check.cargo.not_found = cargo introuvable ({error})
doctor.check.cargo.recommend.install = Installez Rust via rustup pour obtenir cargo.

doctor.check.cache.name = cache cible (target)
doctor.check.cache.missing = dossier target {path} introuvable
doctor.check.cache.recommend.build = Lancez `cargo build` pour initialiser le cache.
doctor.check.cache.write_fail = impossible d'écrire dans {path} ({error})
doctor.check.cache.recommend.perms = Vérifiez les permissions ou régénérez le dossier avec les droits appropriés.
doctor.check.cache.ok = dossier {path} accessible
doctor.check.cache.open_fail = ouverture de {path} impossible ({error})

doctor.check.modules.name = modules embarqués
doctor.check.modules.ok = {path} présent
doctor.check.modules.missing = {path} introuvable
doctor.check.modules.recommend.init = Initialisez les modules via `git submodule update --init --recursive`.

doctor.prompt.purge_cache = Purger le dossier cache {path} ?

doctor.action.cache_purged = (cache target purgé)
doctor.action.cache_kept = (cache conservé)
doctor.action.cache_missing = (aucun cache target détecté)
