from dataclasses import dataclass


@dataclass
class CoreConfig:
    emit_ast: bool = False
    dump_hir: bool = False
    dump_tokens: bool = False
    validate: bool = True

    @staticmethod
    def default() -> "CoreConfig":
        return CoreConfig()
