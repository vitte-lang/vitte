from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class DiagnosticLocale:
    code: str
    name: str
    native_name: str
    aliases: tuple[str, ...]
    nsis_language: str


SUPPORTED_DIAGNOSTIC_LOCALES: tuple[DiagnosticLocale, ...] = (
    DiagnosticLocale("en", "English", "English", ("en-US", "en_GB", "en-GB"), "English"),
    DiagnosticLocale("fr", "French", "Francais", ("fr-FR", "fr_CA", "fr-CA"), "French"),
    DiagnosticLocale("es", "Spanish", "Espanol", ("es-ES", "es_MX", "es-MX"), "Spanish"),
    DiagnosticLocale("de", "German", "Deutsch", ("de-DE", "de_AT", "de-AT"), "German"),
    DiagnosticLocale("it", "Italian", "Italiano", ("it-IT",), "Italian"),
    DiagnosticLocale("pt-BR", "Portuguese (Brazil)", "Portugues do Brasil", ("pt", "pt_BR", "pt-PT"), "PortugueseBR"),
    DiagnosticLocale("nl", "Dutch", "Nederlands", ("nl-NL", "nl_BE", "nl-BE"), "Dutch"),
    DiagnosticLocale("pl", "Polish", "Polski", ("pl-PL",), "Polish"),
    DiagnosticLocale("ru", "Russian", "Russkiy", ("ru-RU",), "Russian"),
    DiagnosticLocale("uk", "Ukrainian", "Ukrainska", ("uk-UA",), "Ukrainian"),
    DiagnosticLocale("zh-CN", "Chinese (Simplified)", "Simplified Chinese", ("zh", "zh_CN", "zh-Hans"), "SimpChinese"),
    DiagnosticLocale("ja", "Japanese", "Nihongo", ("ja-JP",), "Japanese"),
    DiagnosticLocale("ko", "Korean", "Hangugeo", ("ko-KR",), "Korean"),
    DiagnosticLocale("tr", "Turkish", "Turkce", ("tr-TR",), "Turkish"),
    DiagnosticLocale("ar", "Arabic", "Arabic", ("ar-SA", "ar_EG", "ar-EG"), "Arabic"),
)


def supported_locale_codes() -> list[str]:
    return [locale.code for locale in SUPPORTED_DIAGNOSTIC_LOCALES]


def locale_by_code(code: str) -> DiagnosticLocale:
    for locale in SUPPORTED_DIAGNOSTIC_LOCALES:
        if locale.code == code:
            return locale
    raise KeyError(code)

