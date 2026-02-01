from dataclasses import dataclass
from typing import List, Optional


@dataclass
class Diagnostic:
    level: str
    message: str
    span: Optional[str] = None

    def __str__(self) -> str:
        if self.span:
            return f"{self.level}: {self.message} ({self.span})"
        return f"{self.level}: {self.message}"


class DiagnosticBag:
    def __init__(self) -> None:
        self._items: List[Diagnostic] = []

    def error(self, message: str, span: Optional[str] = None) -> None:
        self._items.append(Diagnostic("error", message, span))

    def warning(self, message: str, span: Optional[str] = None) -> None:
        self._items.append(Diagnostic("warning", message, span))

    def note(self, message: str, span: Optional[str] = None) -> None:
        self._items.append(Diagnostic("note", message, span))

    def has_errors(self) -> bool:
        return any(item.level == "error" for item in self._items)

    def __iter__(self):
        return iter(self._items)
