vitte-bootstrap/
  README.md
  LICENSE
  muffin.muf                      # manifest Muffin du workspace bootstrap

  .gitignore
  .editorconfig
  .vscode/
    settings.json
    launch.json
    tasks.json

  scripts/
    bootstrap_stage0.sh           # lance le build du compilateur stage1
    self_host_stage1.sh           # enchaîne stage1 -> stage2
    clean.sh                      # nettoyage complet de target/
    env_local.sh                  # exports PATH, VITTE_BOOTSTRAP_ROOT, etc.

  docs/
    architecture-bootstrap.md     # description du pipeline bootstrap
    stages.md                     # stage0/stage1/stage2, flows de build
    design-decisions.md           # choix de découpage, limites du subset Vitte

  target/                         # artefacts générés (ignoré par git)
    .gitkeep

  tests/
    smoke/
      t_bt_help.vitte             # `vittec0 --help`
      t_bt_build_compiler.vitte   # `vittec0 build -p compiler`
      t_bt_stage_self_host.vitte  # scénario stage1 -> stage2
    data/
      mini_project/
        muffin.muf
        src/l/app/main.vitte      # petit projet Vitte de test
      malformed/
        bad_manifest.muf
        bad_syntax.vitte

  # ---------------------------------------------------------------------------
  # Sources Vitte – bootstrap compiler (binaire `vittec0`)
  # ---------------------------------------------------------------------------
  src/
    l/
      bootstrap/
        bt_main.vitte             # point d’entrée du binaire vittec0

        cli/
          bt_cli.vitte            # parsing global de la ligne de commande
          bt_cmd_build.vitte      # `vittec0 build` (build simple d’un projet)
          bt_cmd_stage.vitte      # `vittec0 stage` (pipelines stage1/stage2)
          bt_cmd_clean.vitte      # `vittec0 clean`
          bt_cmd_help.vitte       # aide détaillée / sous-commandes
          bt_cmd_version.vitte    # version, info toolchain

        core/
          bt_context.vitte        # BootstrapContext (root, target, mode, jobs…)
          bt_options.vitte        # mapping CLI -> BootstrapBuildOptions
          bt_paths.vitte          # résolution chemins (root, target/bootstrap…)
          bt_toolchain.vitte      # description du toolchain hôte + profils
          bt_logging.vitte        # niveaux de logs, formats, prefix `[vittec0]`
          bt_errors.vitte         # types d’erreurs spécifiques au bootstrap
          bt_result.vitte         # Result/Outcome pour passes bootstrap
          bt_profile.vitte        # timings simples des étapes (parse, codegen…)

        front/
          bt_span.vitte           # Span, SourceId, gestion des positions
          bt_tokens.vitte         # enum TokenKind, structure Token
          bt_lexer.vitte          # lexer subset Vitte pour le bootstrap
          bt_ast.vitte            # AST minimal (modules, imports, fn, struct…)
          bt_parser.vitte         # parseur Vitte subset -> bt_ast
          bt_symbol.vitte         # symboles (modules, fonctions, types)
          bt_resolve.vitte        # résolution des noms (module, use, fn)
          bt_diagnostics.vitte    # adaptateur vers diagnostics bootstrap

        middle/
          bt_ir.vitte             # IR intermédiaire “bootstrap” (simplifié)
          bt_lowering.vitte       # AST -> IR
          bt_checks.vitte         # checks structurels/typage simplifiés
          bt_dep_graph.vitte      # graphe de dépendances modules/bootstrap

        back/
          bt_codegen_c.vitte      # (option) génération C pour produire stage1
          bt_codegen_vm.vitte     # (option) génération bytecode VM
          bt_layout.vitte         # organisation des unités de compilation
          bt_emit_files.vitte     # écriture fichiers .c/.vm, drivers, manifests
          bt_driver_c.vitte       # orchestration avec `cc`/`clang` si besoin

        pipeline/
          bt_project_loader.vitte # lecture & validation du muffin.muf cible
          bt_targets.vitte        # targets logiques: compiler, tools, libs
          bt_build_pipeline.vitte # pipeline haut niveau (lex→parse→IR→codegen)
          bt_stage1.vitte         # scénario dédié "build compiler stage1"
          bt_stage2.vitte         # scénario self-host "stage1 -> stage2"

        host/
          bt_fs.vitte             # wrappers FS (open/read/write/list) sur std.fs
          bt_io.vitte             # wrappers IO (print, stderr, progress) sur std.io
          bt_env.vitte            # environnement (PATH, VITTE_CORE_ROOT, etc.)
          bt_process.vitte        # lancement process externe (cc, vitte stage1)
          bt_platform.vitte       # différences OS (paths, exe suffix, etc.)

      # -----------------------------------------------------------------------
      # Mini-std locale (si tu veux un bootstrap très isolé du vrai std)
      # -----------------------------------------------------------------------
      std/
        io/
          std_io.vitte            # print, eprintln, read_to_string
        fs/
          std_fs.vitte            # open, read_all, write_all, mkdirs, exists
        path/
          std_path.vitte          # PathBuf minimal, join, parent, display
        collections/
          std_vec.vitte           # Vec<T> minimal
          std_map.vitte           # Map<K,V> minimal (hash ou tree)
        string/
          std_string.vitte        # String, &str helpers, to_lowercase, split
        time/
          std_time.vitte          # Instant (ms), simple timer