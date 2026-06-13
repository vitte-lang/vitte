#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
LOCALES_DIR = ROOT / "locales"
CORE_CODES = ROOT / "tests/diag_snapshots/core_diagnostic_codes.txt"
sys.path.insert(0, str(ROOT / "tools"))
from diagnostics_locales import locale_by_code, supported_locale_codes
from diagnostic_catalog_data import diagnostic_family, explanation_fields, public_diagnostic_codes, readable_from_code


LOCALE_HEADERS: dict[str, tuple[str, str]] = {
    "en": ("File synchronized from Vitte diagnostic snapshots.", "Keys: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "fr": ("Fichier synchronise depuis les instantanes de diagnostics Vitte.", "Cles : CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "es": ("Archivo sincronizado desde las instantaneas de diagnosticos de Vitte.", "Claves: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "de": ("Datei aus Vitte-Diagnose-Snapshots synchronisiert.", "Schluessel: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "it": ("File sincronizzato dagli snapshot diagnostici di Vitte.", "Chiavi: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "pt-BR": ("Arquivo sincronizado dos snapshots de diagnosticos do Vitte.", "Chaves: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "nl": ("Bestand gesynchroniseerd vanuit Vitte-diagnosesnapshots.", "Sleutels: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "pl": ("Plik zsynchronizowany ze zrzutow diagnostycznych Vitte.", "Klucze: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "ru": ("Fayl sinkhronizirovan iz snimkov diagnostiki Vitte.", "Klyuchi: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "uk": ("Fayl synkhronizovano zi znimkiv diahnostyky Vitte.", "Kliuchi: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "zh-CN": ("文件已从 Vitte 诊断快照同步。", "键: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "ja": ("Vitte 診断スナップショットから同期されたファイルです。", "キー: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "ko": ("Vitte 진단 스냅샷에서 동기화된 파일입니다.", "키: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "tr": ("Dosya Vitte tani anlik goruntulerinden esitlendi.", "Anahtarlar: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
    "ar": ("تمت مزامنة الملف من لقطات تشخيص Vitte.", "المفاتيح: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example"),
}


TERMS: dict[str, dict[str, str]] = {
    "fr": {
        "assignment": "affectation", "argument": "argument", "arguments": "arguments", "assert": "assertion",
        "backend": "backend", "binary": "binaire", "binding": "liaison", "block": "bloc", "borrow": "emprunt",
        "borrowed": "emprunte", "call": "appel", "callee": "appele", "cast": "conversion", "char": "caractere",
        "character": "caractere", "codegen": "generation de code", "condition": "condition", "conflict": "conflit",
        "const": "constante", "constant": "constante", "cycle": "cycle", "declaration": "declaration",
        "delimiter": "delimiteur", "dereference": "dereferencement", "diagnostics": "diagnostics",
        "division": "division", "drop": "destruction", "duplicate": "duplique", "empty": "vide",
        "end": "fin", "entry": "entree", "escape": "echappement", "expression": "expression",
        "failed": "echec", "feature": "fonctionnalite", "field": "champ", "file": "fichier",
        "float": "flottant", "flow": "flux", "format": "format", "found": "trouve", "generic": "generique",
        "identifier": "identifiant", "import": "import", "indentation": "indentation", "index": "index",
        "input": "entree", "instruction": "instruction", "internal": "interne", "invalid": "invalide",
        "item": "element", "lifetime": "duree de vie", "literal": "litteral", "local": "local",
        "member": "membre", "mismatch": "incompatibilite", "missing": "manquant", "module": "module",
        "move": "deplacement", "moved": "deplace", "mutable": "mutable", "name": "nom",
        "node": "noeud", "null": "nul", "number": "nombre", "object": "objet", "operand": "operande",
        "operator": "operateur", "output": "sortie", "overflow": "depassement", "panic": "panique",
        "parser": "parseur", "path": "chemin", "pattern": "motif", "profile": "profil", "reference": "reference",
        "return": "retour", "rule": "regle", "shared": "partage", "source": "source", "stack": "pile",
        "statement": "instruction", "string": "chaine", "symbol": "symbole", "syntax": "syntaxe",
        "target": "cible", "terminator": "terminateur", "token": "jeton", "tool": "outil",
        "too": "trop", "type": "type", "undefined": "indefini", "unexpected": "inattendu",
        "uninitialized": "non initialise", "unknown": "inconnu", "unreachable": "inatteignable",
        "unsupported": "non pris en charge", "use": "utilisation", "used": "utilise", "value": "valeur",
        "visibility": "visibilite", "while": "pendant", "write": "ecriture", "zero": "zero",
    },
    "es": {
        "assignment": "asignacion", "argument": "argumento", "arguments": "argumentos", "assert": "asercion",
        "backend": "backend", "binary": "binario", "binding": "enlace", "block": "bloque", "borrow": "prestamo",
        "borrowed": "prestado", "call": "llamada", "callee": "destinatario", "cast": "conversion", "char": "caracter",
        "character": "caracter", "codegen": "generacion de codigo", "condition": "condicion", "conflict": "conflicto",
        "const": "constante", "constant": "constante", "cycle": "ciclo", "declaration": "declaracion",
        "delimiter": "delimitador", "dereference": "desreferencia", "diagnostics": "diagnosticos",
        "division": "division", "drop": "destruccion", "duplicate": "duplicado", "empty": "vacio",
        "end": "fin", "entry": "entrada", "escape": "escape", "expression": "expresion", "failed": "fallido",
        "feature": "funcion", "field": "campo", "file": "archivo", "float": "flotante", "flow": "flujo",
        "format": "formato", "found": "encontrado", "generic": "generico", "identifier": "identificador",
        "import": "importacion", "indentation": "sangria", "index": "indice", "input": "entrada",
        "instruction": "instruccion", "internal": "interno", "invalid": "invalido", "item": "elemento",
        "lifetime": "vida", "literal": "literal", "local": "local", "member": "miembro",
        "mismatch": "incompatibilidad", "missing": "faltante", "module": "modulo", "move": "movimiento",
        "moved": "movido", "mutable": "mutable", "name": "nombre", "node": "nodo", "null": "nulo",
        "number": "numero", "object": "objeto", "operand": "operando", "operator": "operador",
        "output": "salida", "overflow": "desbordamiento", "panic": "panico", "parser": "analizador",
        "path": "ruta", "pattern": "patron", "profile": "perfil", "reference": "referencia",
        "return": "retorno", "rule": "regla", "shared": "compartido", "source": "fuente", "stack": "pila",
        "statement": "sentencia", "string": "cadena", "symbol": "simbolo", "syntax": "sintaxis",
        "target": "destino", "terminator": "terminador", "token": "token", "tool": "herramienta",
        "too": "demasiado", "type": "tipo", "undefined": "indefinido", "unexpected": "inesperado",
        "uninitialized": "no inicializado", "unknown": "desconocido", "unreachable": "inalcanzable",
        "unsupported": "no compatible", "use": "uso", "used": "usado", "value": "valor",
        "visibility": "visibilidad", "while": "mientras", "write": "escritura", "zero": "cero",
    },
    "de": {
        "assignment": "Zuweisung", "argument": "Argument", "arguments": "Argumente", "assert": "Assertion",
        "backend": "Backend", "binary": "binaer", "binding": "Bindung", "block": "Block", "borrow": "Ausleihe",
        "borrowed": "ausgeliehen", "call": "Aufruf", "callee": "Aufrufziel", "cast": "Umwandlung", "char": "Zeichen",
        "character": "Zeichen", "codegen": "Codeerzeugung", "condition": "Bedingung", "conflict": "Konflikt",
        "const": "Konstante", "constant": "Konstante", "cycle": "Zyklus", "declaration": "Deklaration",
        "delimiter": "Trennzeichen", "dereference": "Dereferenzierung", "diagnostics": "Diagnosen",
        "division": "Division", "drop": "Freigabe", "duplicate": "doppelt", "empty": "leer",
        "end": "Ende", "entry": "Eintrag", "escape": "Escape", "expression": "Ausdruck", "failed": "fehlgeschlagen",
        "feature": "Funktion", "field": "Feld", "file": "Datei", "float": "Fliesszahl", "flow": "Kontrollfluss",
        "format": "Format", "found": "gefunden", "generic": "generisch", "identifier": "Bezeichner",
        "import": "Import", "indentation": "Einrueckung", "index": "Index", "input": "Eingabe",
        "instruction": "Instruktion", "internal": "intern", "invalid": "ungueltig", "item": "Element",
        "lifetime": "Lebensdauer", "literal": "Literal", "local": "lokal", "member": "Member",
        "mismatch": "Nichtuebereinstimmung", "missing": "fehlend", "module": "Modul", "move": "Verschiebung",
        "moved": "verschoben", "mutable": "veraenderbar", "name": "Name", "node": "Knoten", "null": "Null",
        "number": "Zahl", "object": "Objekt", "operand": "Operand", "operator": "Operator",
        "output": "Ausgabe", "overflow": "Ueberlauf", "panic": "Panik", "parser": "Parser",
        "path": "Pfad", "pattern": "Muster", "profile": "Profil", "reference": "Referenz",
        "return": "Rueckgabe", "rule": "Regel", "shared": "geteilt", "source": "Quelle", "stack": "Stack",
        "statement": "Anweisung", "string": "Zeichenkette", "symbol": "Symbol", "syntax": "Syntax",
        "target": "Ziel", "terminator": "Terminator", "token": "Token", "tool": "Werkzeug",
        "too": "zu", "type": "Typ", "undefined": "undefiniert", "unexpected": "unerwartet",
        "uninitialized": "nicht initialisiert", "unknown": "unbekannt", "unreachable": "unerreichbar",
        "unsupported": "nicht unterstuetzt", "use": "Verwendung", "used": "verwendet", "value": "Wert",
        "visibility": "Sichtbarkeit", "while": "waehrend", "write": "Schreibzugriff", "zero": "Null",
    },
    "it": {
        "assignment": "assegnazione", "argument": "argomento", "arguments": "argomenti", "assert": "asserzione",
        "backend": "backend", "binary": "binario", "binding": "associazione", "block": "blocco", "borrow": "prestito",
        "borrowed": "preso in prestito", "call": "chiamata", "callee": "destinatario", "cast": "conversione",
        "char": "carattere", "character": "carattere", "codegen": "generazione codice", "condition": "condizione",
        "conflict": "conflitto", "const": "costante", "constant": "costante", "cycle": "ciclo",
        "declaration": "dichiarazione", "delimiter": "delimitatore", "dereference": "dereferenziazione",
        "diagnostics": "diagnostica", "division": "divisione", "drop": "rilascio", "duplicate": "duplicato",
        "empty": "vuoto", "end": "fine", "entry": "voce", "escape": "escape", "expression": "espressione",
        "failed": "fallito", "feature": "funzionalita", "field": "campo", "file": "file", "float": "virgola mobile",
        "flow": "flusso", "format": "formato", "found": "trovato", "generic": "generico",
        "identifier": "identificatore", "import": "importazione", "indentation": "indentazione", "index": "indice",
        "input": "input", "instruction": "istruzione", "internal": "interno", "invalid": "non valido",
        "item": "elemento", "lifetime": "durata", "literal": "letterale", "local": "locale", "member": "membro",
        "mismatch": "incompatibilita", "missing": "mancante", "module": "modulo", "move": "spostamento",
        "moved": "spostato", "mutable": "mutabile", "name": "nome", "node": "nodo", "null": "nullo",
        "number": "numero", "object": "oggetto", "operand": "operando", "operator": "operatore",
        "output": "output", "overflow": "overflow", "panic": "panic", "parser": "parser", "path": "percorso",
        "pattern": "pattern", "profile": "profilo", "reference": "riferimento", "return": "ritorno",
        "rule": "regola", "shared": "condiviso", "source": "sorgente", "stack": "stack", "statement": "istruzione",
        "string": "stringa", "symbol": "simbolo", "syntax": "sintassi", "target": "destinazione",
        "terminator": "terminatore", "token": "token", "tool": "strumento", "too": "troppo", "type": "tipo",
        "undefined": "non definito", "unexpected": "inatteso", "uninitialized": "non inizializzato",
        "unknown": "sconosciuto", "unreachable": "irraggiungibile", "unsupported": "non supportato",
        "use": "uso", "used": "usato", "value": "valore", "visibility": "visibilita", "while": "durante",
        "write": "scrittura", "zero": "zero",
    },
    "pt-BR": {
        "assignment": "atribuicao", "argument": "argumento", "arguments": "argumentos", "assert": "assercao",
        "backend": "backend", "binary": "binario", "binding": "vinculo", "block": "bloco", "borrow": "emprestimo",
        "borrowed": "emprestado", "call": "chamada", "callee": "destino da chamada", "cast": "conversao",
        "char": "caractere", "character": "caractere", "codegen": "geracao de codigo", "condition": "condicao",
        "conflict": "conflito", "const": "constante", "constant": "constante", "cycle": "ciclo",
        "declaration": "declaracao", "delimiter": "delimitador", "dereference": "desreferencia",
        "diagnostics": "diagnosticos", "division": "divisao", "drop": "descarte", "duplicate": "duplicado",
        "empty": "vazio", "end": "fim", "entry": "entrada", "escape": "escape", "expression": "expressao",
        "failed": "falhou", "feature": "recurso", "field": "campo", "file": "arquivo", "float": "ponto flutuante",
        "flow": "fluxo", "format": "formato", "found": "encontrado", "generic": "generico",
        "identifier": "identificador", "import": "importacao", "indentation": "indentacao", "index": "indice",
        "input": "entrada", "instruction": "instrucao", "internal": "interno", "invalid": "invalido",
        "item": "item", "lifetime": "tempo de vida", "literal": "literal", "local": "local", "member": "membro",
        "mismatch": "incompatibilidade", "missing": "ausente", "module": "modulo", "move": "movimento",
        "moved": "movido", "mutable": "mutavel", "name": "nome", "node": "no", "null": "nulo",
        "number": "numero", "object": "objeto", "operand": "operando", "operator": "operador",
        "output": "saida", "overflow": "estouro", "panic": "panico", "parser": "parser", "path": "caminho",
        "pattern": "padrao", "profile": "perfil", "reference": "referencia", "return": "retorno",
        "rule": "regra", "shared": "compartilhado", "source": "fonte", "stack": "pilha", "statement": "instrucao",
        "string": "string", "symbol": "simbolo", "syntax": "sintaxe", "target": "destino",
        "terminator": "terminador", "token": "token", "tool": "ferramenta", "too": "demais", "type": "tipo",
        "undefined": "indefinido", "unexpected": "inesperado", "uninitialized": "nao inicializado",
        "unknown": "desconhecido", "unreachable": "inalcancavel", "unsupported": "nao suportado",
        "use": "uso", "used": "usado", "value": "valor", "visibility": "visibilidade", "while": "enquanto",
        "write": "escrita", "zero": "zero",
    },
    "nl": {
        "assignment": "toewijzing", "argument": "argument", "arguments": "argumenten", "assert": "assertie",
        "backend": "backend", "binary": "binair", "binding": "binding", "block": "blok", "borrow": "lening",
        "borrowed": "geleend", "call": "aanroep", "callee": "aanroepdoel", "cast": "conversie",
        "char": "teken", "character": "teken", "codegen": "codegeneratie", "condition": "voorwaarde",
        "conflict": "conflict", "const": "constante", "constant": "constante", "cycle": "cyclus",
        "declaration": "declaratie", "delimiter": "scheidingsteken", "dereference": "dereferentie",
        "diagnostics": "diagnostiek", "division": "deling", "drop": "vrijgave", "duplicate": "dubbel",
        "empty": "leeg", "end": "einde", "entry": "ingang", "escape": "escape", "expression": "expressie",
        "failed": "mislukt", "feature": "functie", "field": "veld", "file": "bestand", "float": "kommagetal",
        "flow": "stroom", "format": "formaat", "found": "gevonden", "generic": "generiek",
        "identifier": "identifier", "import": "import", "indentation": "inspringing", "index": "index",
        "input": "invoer", "instruction": "instructie", "internal": "intern", "invalid": "ongeldig",
        "item": "item", "lifetime": "levensduur", "literal": "literal", "local": "lokaal", "member": "lid",
        "mismatch": "mismatch", "missing": "ontbrekend", "module": "module", "move": "verplaatsing",
        "moved": "verplaatst", "mutable": "muteerbaar", "name": "naam", "node": "knoop", "null": "nul",
        "number": "nummer", "object": "object", "operand": "operand", "operator": "operator",
        "output": "uitvoer", "overflow": "overloop", "panic": "panic", "parser": "parser", "path": "pad",
        "pattern": "patroon", "profile": "profiel", "reference": "referentie", "return": "return",
        "rule": "regel", "shared": "gedeeld", "source": "bron", "stack": "stack", "statement": "statement",
        "string": "string", "symbol": "symbool", "syntax": "syntaxis", "target": "doel",
        "terminator": "terminator", "token": "token", "tool": "hulpmiddel", "too": "te", "type": "type",
        "undefined": "ongedefinieerd", "unexpected": "onverwacht", "uninitialized": "niet geinitialiseerd",
        "unknown": "onbekend", "unreachable": "onbereikbaar", "unsupported": "niet ondersteund",
        "use": "gebruik", "used": "gebruikt", "value": "waarde", "visibility": "zichtbaarheid", "while": "terwijl",
        "write": "schrijven", "zero": "nul",
    },
    "pl": {
        "assignment": "przypisanie", "argument": "argument", "arguments": "argumenty", "assert": "asercja",
        "backend": "backend", "binary": "binarny", "binding": "wiazanie", "block": "blok", "borrow": "pozyczenie",
        "borrowed": "pozyczony", "call": "wywolanie", "callee": "cel wywolania", "cast": "rzutowanie",
        "char": "znak", "character": "znak", "codegen": "generowanie kodu", "condition": "warunek",
        "conflict": "konflikt", "const": "stala", "constant": "stala", "cycle": "cykl", "declaration": "deklaracja",
        "delimiter": "separator", "dereference": "dereferencja", "diagnostics": "diagnostyka",
        "division": "dzielenie", "drop": "zwolnienie", "duplicate": "duplikat", "empty": "pusty",
        "end": "koniec", "entry": "wejscie", "escape": "escape", "expression": "wyrazenie", "failed": "niepowodzenie",
        "feature": "funkcja", "field": "pole", "file": "plik", "float": "zmiennoprzecinkowy", "flow": "przeplyw",
        "format": "format", "found": "znaleziono", "generic": "generyczny", "identifier": "identyfikator",
        "import": "import", "indentation": "wciecie", "index": "indeks", "input": "wejscie",
        "instruction": "instrukcja", "internal": "wewnetrzny", "invalid": "nieprawidlowy", "item": "element",
        "lifetime": "czas zycia", "literal": "literal", "local": "lokalny", "member": "skladnik",
        "mismatch": "niezgodnosc", "missing": "brakujacy", "module": "modul", "move": "przeniesienie",
        "moved": "przeniesiony", "mutable": "mutowalny", "name": "nazwa", "node": "wezel", "null": "null",
        "number": "liczba", "object": "obiekt", "operand": "operand", "operator": "operator",
        "output": "wyjscie", "overflow": "przepelnienie", "panic": "panika", "parser": "parser", "path": "sciezka",
        "pattern": "wzorzec", "profile": "profil", "reference": "referencja", "return": "zwrot",
        "rule": "regula", "shared": "wspoldzielony", "source": "zrodlo", "stack": "stos", "statement": "instrukcja",
        "string": "ciag", "symbol": "symbol", "syntax": "skladnia", "target": "cel",
        "terminator": "terminator", "token": "token", "tool": "narzedzie", "too": "zbyt", "type": "typ",
        "undefined": "niezdefiniowany", "unexpected": "nieoczekiwany", "uninitialized": "niezainicjalizowany",
        "unknown": "nieznany", "unreachable": "nieosiagalny", "unsupported": "nieobslugiwany",
        "use": "uzycie", "used": "uzyty", "value": "wartosc", "visibility": "widocznosc", "while": "podczas",
        "write": "zapis", "zero": "zero",
    },
    "ru": {
        "assignment": "prisvaivanie", "argument": "argument", "arguments": "argumenty", "assert": "proverka",
        "backend": "bekend", "binary": "binarnyi", "binding": "privyazka", "block": "blok", "borrow": "zaimstvovanie",
        "borrowed": "zaimstvovannyi", "call": "vyzov", "callee": "tsel vyzova", "cast": "preobrazovanie",
        "char": "simvol", "character": "simvol", "codegen": "generatsiya koda", "condition": "uslovie",
        "conflict": "konflikt", "const": "konstanta", "constant": "konstanta", "cycle": "tsikl",
        "declaration": "obyavlenie", "delimiter": "razdelitel", "dereference": "razymenovanie",
        "diagnostics": "diagnostika", "division": "delenie", "drop": "osvobozhdenie", "duplicate": "dublikat",
        "empty": "pustoi", "end": "konets", "entry": "tochka vhoda", "escape": "escape", "expression": "vyrazhenie",
        "failed": "sboi", "feature": "vozmozhnost", "field": "pole", "file": "fail", "float": "chislo s plavayushchei tochkoi",
        "flow": "potok", "format": "format", "found": "naideno", "generic": "obobshchennyi",
        "identifier": "identifikator", "import": "import", "indentation": "otstup", "index": "indeks",
        "input": "vvod", "instruction": "instruktsiya", "internal": "vnutrennii", "invalid": "nedopustimyi",
        "item": "element", "lifetime": "vremya zhizni", "literal": "literal", "local": "lokalnyi", "member": "chlen",
        "mismatch": "nesootvetstvie", "missing": "otsutstvuet", "module": "modul", "move": "peremeshchenie",
        "moved": "peremeshchen", "mutable": "izmenyaemyi", "name": "imya", "node": "uzel", "null": "null",
        "number": "chislo", "object": "obekt", "operand": "operand", "operator": "operator",
        "output": "vyvod", "overflow": "perepolnenie", "panic": "panika", "parser": "parser", "path": "put",
        "pattern": "shablon", "profile": "profil", "reference": "ssylka", "return": "vozvrat",
        "rule": "pravilo", "shared": "obshchii", "source": "istochnik", "stack": "stek", "statement": "operator",
        "string": "stroka", "symbol": "simvol", "syntax": "sintaksis", "target": "tsel",
        "terminator": "terminator", "token": "token", "tool": "instrument", "too": "slishkom", "type": "tip",
        "undefined": "neopredelen", "unexpected": "neozhidannyi", "uninitialized": "ne initsializirovan",
        "unknown": "neizvestnyi", "unreachable": "nedostizhimyi", "unsupported": "ne podderzhivaetsya",
        "use": "ispolzovanie", "used": "ispolzovan", "value": "znachenie", "visibility": "vidimost", "while": "vo vremya",
        "write": "zapis", "zero": "nol",
    },
    "uk": {
        "assignment": "prysvoiennia", "argument": "argument", "arguments": "argumenty", "assert": "perevirka",
        "backend": "bekend", "binary": "binarnyi", "binding": "pryviazka", "block": "blok", "borrow": "pozychennia",
        "borrowed": "pozychenyi", "call": "vyklyk", "callee": "tsil vyklyku", "cast": "peretvorennia",
        "char": "symvol", "character": "symvol", "codegen": "heneratsiia kodu", "condition": "umova",
        "conflict": "konflikt", "const": "konstanta", "constant": "konstanta", "cycle": "tsykl",
        "declaration": "oholoshennia", "delimiter": "rozdilnyk", "dereference": "rozimenovuvannia",
        "diagnostics": "diahnostyka", "division": "dilennia", "drop": "zvilnennia", "duplicate": "dublikat",
        "empty": "porozhnii", "end": "kinets", "entry": "vkhid", "escape": "escape", "expression": "vyraz",
        "failed": "zbi", "feature": "mozhlyvist", "field": "pole", "file": "fail", "float": "drobove chyslo",
        "flow": "potik", "format": "format", "found": "znaideno", "generic": "uzahalnenyi",
        "identifier": "identyfikator", "import": "import", "indentation": "vidstup", "index": "indeks",
        "input": "vkhid", "instruction": "instruktsiia", "internal": "vnutrishnii", "invalid": "neprypustymyi",
        "item": "element", "lifetime": "chas zhyttia", "literal": "literal", "local": "lokalnyi", "member": "chlen",
        "mismatch": "nevidpovidnist", "missing": "vidsutnii", "module": "modul", "move": "peremishchennia",
        "moved": "peremishchenyi", "mutable": "zminnyi", "name": "imia", "node": "vuzol", "null": "null",
        "number": "chyslo", "object": "obiekt", "operand": "operand", "operator": "operator",
        "output": "vykhid", "overflow": "perepovnennia", "panic": "panika", "parser": "parser", "path": "shliakh",
        "pattern": "zrazok", "profile": "profil", "reference": "posylannia", "return": "povernennia",
        "rule": "pravylo", "shared": "spilnyi", "source": "dzherelo", "stack": "stek", "statement": "operator",
        "string": "riadok", "symbol": "symvol", "syntax": "syntaksys", "target": "tsil",
        "terminator": "terminator", "token": "token", "tool": "instrument", "too": "zanadto", "type": "typ",
        "undefined": "nevyznachenyi", "unexpected": "neochikuvanyi", "uninitialized": "ne initsializovanyi",
        "unknown": "nevidomyi", "unreachable": "nedosiazhnyi", "unsupported": "ne pidtrymuietsia",
        "use": "vykorystannia", "used": "vykorystano", "value": "znachennia", "visibility": "vydymist", "while": "pid chas",
        "write": "zapys", "zero": "nul",
    },
    "zh-CN": {
        "assignment": "赋值", "argument": "参数", "arguments": "参数", "assert": "断言",
        "backend": "后端", "binary": "二进制", "binding": "绑定", "block": "块", "borrow": "借用",
        "borrowed": "已借用", "call": "调用", "callee": "被调用项", "cast": "转换", "char": "字符",
        "character": "字符", "codegen": "代码生成", "condition": "条件", "conflict": "冲突",
        "const": "常量", "constant": "常量", "cycle": "循环", "declaration": "声明",
        "delimiter": "分隔符", "dereference": "解引用", "diagnostics": "诊断", "division": "除法",
        "drop": "释放", "duplicate": "重复", "empty": "空", "end": "结束", "entry": "入口",
        "escape": "转义", "expression": "表达式", "failed": "失败", "feature": "功能", "field": "字段",
        "file": "文件", "float": "浮点数", "flow": "流程", "format": "格式", "found": "找到",
        "generic": "泛型", "identifier": "标识符", "import": "导入", "indentation": "缩进", "index": "索引",
        "input": "输入", "instruction": "指令", "internal": "内部", "invalid": "无效", "item": "项",
        "lifetime": "生命周期", "literal": "字面量", "local": "局部", "member": "成员",
        "mismatch": "不匹配", "missing": "缺失", "module": "模块", "move": "移动", "moved": "已移动",
        "mutable": "可变", "name": "名称", "node": "节点", "null": "空指针", "number": "数字",
        "object": "对象", "operand": "操作数", "operator": "运算符", "output": "输出",
        "overflow": "溢出", "panic": "恐慌", "parser": "解析器", "path": "路径", "pattern": "模式",
        "profile": "配置", "reference": "引用", "return": "返回", "rule": "规则", "shared": "共享",
        "source": "源", "stack": "栈", "statement": "语句", "string": "字符串", "symbol": "符号",
        "syntax": "语法", "target": "目标", "terminator": "终止符", "token": "标记", "tool": "工具",
        "too": "过于", "type": "类型", "undefined": "未定义", "unexpected": "意外",
        "uninitialized": "未初始化", "unknown": "未知", "unreachable": "不可达", "unsupported": "不支持",
        "use": "使用", "used": "已使用", "value": "值", "visibility": "可见性", "while": "当",
        "write": "写入", "zero": "零",
    },
    "ja": {
        "assignment": "代入", "argument": "引数", "arguments": "引数", "assert": "アサート",
        "backend": "バックエンド", "binary": "バイナリ", "binding": "束縛", "block": "ブロック",
        "borrow": "借用", "borrowed": "借用済み", "call": "呼び出し", "callee": "呼び出し先",
        "cast": "キャスト", "char": "文字", "character": "文字", "codegen": "コード生成",
        "condition": "条件", "conflict": "競合", "const": "定数", "constant": "定数", "cycle": "循環",
        "declaration": "宣言", "delimiter": "区切り", "dereference": "参照外し", "diagnostics": "診断",
        "division": "除算", "drop": "破棄", "duplicate": "重複", "empty": "空", "end": "終了",
        "entry": "エントリ", "escape": "エスケープ", "expression": "式", "failed": "失敗",
        "feature": "機能", "field": "フィールド", "file": "ファイル", "float": "浮動小数",
        "flow": "フロー", "format": "形式", "found": "検出", "generic": "ジェネリック",
        "identifier": "識別子", "import": "インポート", "indentation": "インデント", "index": "インデックス",
        "input": "入力", "instruction": "命令", "internal": "内部", "invalid": "無効", "item": "項目",
        "lifetime": "ライフタイム", "literal": "リテラル", "local": "ローカル", "member": "メンバー",
        "mismatch": "不一致", "missing": "欠落", "module": "モジュール", "move": "ムーブ",
        "moved": "ムーブ済み", "mutable": "可変", "name": "名前", "node": "ノード", "null": "ヌル",
        "number": "数値", "object": "オブジェクト", "operand": "オペランド", "operator": "演算子",
        "output": "出力", "overflow": "オーバーフロー", "panic": "パニック", "parser": "パーサ",
        "path": "パス", "pattern": "パターン", "profile": "プロファイル", "reference": "参照",
        "return": "戻り", "rule": "規則", "shared": "共有", "source": "ソース", "stack": "スタック",
        "statement": "文", "string": "文字列", "symbol": "シンボル", "syntax": "構文", "target": "対象",
        "terminator": "終端", "token": "トークン", "tool": "ツール", "too": "過剰", "type": "型",
        "undefined": "未定義", "unexpected": "予期しない", "uninitialized": "未初期化",
        "unknown": "不明", "unreachable": "到達不能", "unsupported": "未対応", "use": "使用",
        "used": "使用済み", "value": "値", "visibility": "可視性", "while": "中", "write": "書き込み",
        "zero": "ゼロ",
    },
    "ko": {
        "assignment": "대입", "argument": "인수", "arguments": "인수", "assert": "단언",
        "backend": "백엔드", "binary": "바이너리", "binding": "바인딩", "block": "블록", "borrow": "빌림",
        "borrowed": "빌린 값", "call": "호출", "callee": "호출 대상", "cast": "변환", "char": "문자",
        "character": "문자", "codegen": "코드 생성", "condition": "조건", "conflict": "충돌",
        "const": "상수", "constant": "상수", "cycle": "순환", "declaration": "선언", "delimiter": "구분자",
        "dereference": "역참조", "diagnostics": "진단", "division": "나눗셈", "drop": "해제",
        "duplicate": "중복", "empty": "비어 있음", "end": "끝", "entry": "진입점", "escape": "이스케이프",
        "expression": "식", "failed": "실패", "feature": "기능", "field": "필드", "file": "파일",
        "float": "부동소수", "flow": "흐름", "format": "형식", "found": "발견됨", "generic": "제네릭",
        "identifier": "식별자", "import": "가져오기", "indentation": "들여쓰기", "index": "인덱스",
        "input": "입력", "instruction": "명령", "internal": "내부", "invalid": "잘못됨", "item": "항목",
        "lifetime": "수명", "literal": "리터럴", "local": "로컬", "member": "멤버",
        "mismatch": "불일치", "missing": "누락", "module": "모듈", "move": "이동", "moved": "이동됨",
        "mutable": "변경 가능", "name": "이름", "node": "노드", "null": "널", "number": "숫자",
        "object": "객체", "operand": "피연산자", "operator": "연산자", "output": "출력",
        "overflow": "오버플로", "panic": "패닉", "parser": "파서", "path": "경로", "pattern": "패턴",
        "profile": "프로필", "reference": "참조", "return": "반환", "rule": "규칙", "shared": "공유",
        "source": "소스", "stack": "스택", "statement": "문", "string": "문자열", "symbol": "심볼",
        "syntax": "구문", "target": "대상", "terminator": "종결자", "token": "토큰", "tool": "도구",
        "too": "너무", "type": "타입", "undefined": "정의되지 않음", "unexpected": "예상치 못한",
        "uninitialized": "초기화되지 않음", "unknown": "알 수 없음", "unreachable": "도달 불가",
        "unsupported": "지원되지 않음", "use": "사용", "used": "사용됨", "value": "값",
        "visibility": "가시성", "while": "동안", "write": "쓰기", "zero": "영",
    },
    "tr": {
        "assignment": "atama", "argument": "arguman", "arguments": "argumanlar", "assert": "dogrulama",
        "backend": "arka uc", "binary": "ikili", "binding": "baglama", "block": "blok", "borrow": "odunc",
        "borrowed": "odunc alinmis", "call": "cagri", "callee": "cagri hedefi", "cast": "donusum",
        "char": "karakter", "character": "karakter", "codegen": "kod uretimi", "condition": "kosul",
        "conflict": "cakismazlik", "const": "sabit", "constant": "sabit", "cycle": "dongu",
        "declaration": "bildirim", "delimiter": "ayirici", "dereference": "basvuru cozme",
        "diagnostics": "tanilar", "division": "bolme", "drop": "birakma", "duplicate": "yinelenen",
        "empty": "bos", "end": "son", "entry": "giris", "escape": "kacis", "expression": "ifade",
        "failed": "basarisiz", "feature": "ozellik", "field": "alan", "file": "dosya", "float": "ondalik",
        "flow": "akis", "format": "bicim", "found": "bulundu", "generic": "jenerik",
        "identifier": "tanimlayici", "import": "ice aktarma", "indentation": "girinti", "index": "indeks",
        "input": "girdi", "instruction": "komut", "internal": "ic", "invalid": "gecersiz", "item": "oge",
        "lifetime": "yasam suresi", "literal": "literal", "local": "yerel", "member": "uye",
        "mismatch": "uyusmazlik", "missing": "eksik", "module": "modul", "move": "tasima",
        "moved": "tasinmis", "mutable": "degistirilebilir", "name": "ad", "node": "dugum", "null": "null",
        "number": "sayi", "object": "nesne", "operand": "operand", "operator": "operator",
        "output": "cikti", "overflow": "tasma", "panic": "panik", "parser": "ayristirici", "path": "yol",
        "pattern": "desen", "profile": "profil", "reference": "basvuru", "return": "donus",
        "rule": "kural", "shared": "paylasilan", "source": "kaynak", "stack": "yigin", "statement": "deyim",
        "string": "dize", "symbol": "sembol", "syntax": "soz dizimi", "target": "hedef",
        "terminator": "sonlandirici", "token": "token", "tool": "arac", "too": "cok", "type": "tur",
        "undefined": "tanimlanmamis", "unexpected": "beklenmeyen", "uninitialized": "baslatilmamis",
        "unknown": "bilinmeyen", "unreachable": "ulasilamaz", "unsupported": "desteklenmeyen",
        "use": "kullanim", "used": "kullanildi", "value": "deger", "visibility": "gorunurluk", "while": "sirasinda",
        "write": "yazma", "zero": "sifir",
    },
    "ar": {
        "assignment": "تعيين", "argument": "وسيط", "arguments": "وسائط", "assert": "تحقق",
        "backend": "الخلفية", "binary": "ثنائي", "binding": "ربط", "block": "كتلة", "borrow": "استعارة",
        "borrowed": "مستعار", "call": "استدعاء", "callee": "هدف الاستدعاء", "cast": "تحويل",
        "char": "حرف", "character": "حرف", "codegen": "توليد الكود", "condition": "شرط",
        "conflict": "تعارض", "const": "ثابت", "constant": "ثابت", "cycle": "دورة", "declaration": "تصريح",
        "delimiter": "فاصل", "dereference": "فك مرجع", "diagnostics": "تشخيصات", "division": "قسمة",
        "drop": "اسقاط", "duplicate": "مكرر", "empty": "فارغ", "end": "نهاية", "entry": "مدخل",
        "escape": "هروب", "expression": "تعبير", "failed": "فشل", "feature": "ميزة", "field": "حقل",
        "file": "ملف", "float": "عدد عشري", "flow": "تدفق", "format": "تنسيق", "found": "موجود",
        "generic": "عام", "identifier": "معرف", "import": "استيراد", "indentation": "ازاحة", "index": "فهرس",
        "input": "ادخال", "instruction": "تعليمة", "internal": "داخلي", "invalid": "غير صالح",
        "item": "عنصر", "lifetime": "مدة حياة", "literal": "قيمة حرفية", "local": "محلي", "member": "عضو",
        "mismatch": "عدم تطابق", "missing": "مفقود", "module": "وحدة", "move": "نقل", "moved": "منقول",
        "mutable": "قابل للتغيير", "name": "اسم", "node": "عقدة", "null": "فارغ", "number": "رقم",
        "object": "كائن", "operand": "معامل", "operator": "عامل", "output": "اخراج", "overflow": "فيض",
        "panic": "هلع", "parser": "محلل", "path": "مسار", "pattern": "نمط", "profile": "ملف تعريف",
        "reference": "مرجع", "return": "ارجاع", "rule": "قاعدة", "shared": "مشترك", "source": "مصدر",
        "stack": "مكدس", "statement": "عبارة", "string": "سلسلة", "symbol": "رمز", "syntax": "بناء",
        "target": "هدف", "terminator": "منهي", "token": "رمز لغوي", "tool": "اداة", "too": "زائد",
        "type": "نوع", "undefined": "غير معرف", "unexpected": "غير متوقع", "uninitialized": "غير مهيأ",
        "unknown": "مجهول", "unreachable": "غير قابل للوصول", "unsupported": "غير مدعوم",
        "use": "استخدام", "used": "مستخدم", "value": "قيمة", "visibility": "مرئية", "while": "اثناء",
        "write": "كتابة", "zero": "صفر",
    },
}

MODIFIER_TEMPLATES: dict[str, dict[str, str]] = {
    "fr": {
        "expected": "{term} attendu",
        "invalid": "{term} invalide",
        "unexpected": "{term} inattendu",
        "unknown": "{term} inconnu",
        "unsupported": "{term} non pris en charge",
        "duplicate": "{term} duplique",
        "missing": "{term} manquant",
        "unclosed": "{term} non ferme",
        "unterminated": "{term} non termine",
    },
    "es": {
        "expected": "se esperaba {term}",
        "invalid": "{term} invalido",
        "unexpected": "{term} inesperado",
        "unknown": "{term} desconocido",
        "unsupported": "{term} no compatible",
        "duplicate": "{term} duplicado",
        "missing": "{term} faltante",
        "unclosed": "{term} sin cerrar",
        "unterminated": "{term} sin terminar",
    },
    "de": {
        "expected": "{term} erwartet",
        "invalid": "{term} ungueltig",
        "unexpected": "{term} unerwartet",
        "unknown": "{term} unbekannt",
        "unsupported": "{term} nicht unterstuetzt",
        "duplicate": "{term} doppelt",
        "missing": "{term} fehlt",
        "unclosed": "{term} nicht geschlossen",
        "unterminated": "{term} nicht beendet",
    },
    "it": {
        "expected": "{term} atteso",
        "invalid": "{term} non valido",
        "unexpected": "{term} inatteso",
        "unknown": "{term} sconosciuto",
        "unsupported": "{term} non supportato",
        "duplicate": "{term} duplicato",
        "missing": "{term} mancante",
        "unclosed": "{term} non chiuso",
        "unterminated": "{term} non terminato",
    },
    "pt-BR": {
        "expected": "{term} esperado",
        "invalid": "{term} invalido",
        "unexpected": "{term} inesperado",
        "unknown": "{term} desconhecido",
        "unsupported": "{term} nao suportado",
        "duplicate": "{term} duplicado",
        "missing": "{term} ausente",
        "unclosed": "{term} nao fechado",
        "unterminated": "{term} nao terminado",
    },
    "nl": {
        "expected": "{term} verwacht",
        "invalid": "{term} ongeldig",
        "unexpected": "{term} onverwacht",
        "unknown": "{term} onbekend",
        "unsupported": "{term} niet ondersteund",
        "duplicate": "{term} dubbel",
        "missing": "{term} ontbreekt",
        "unclosed": "{term} niet gesloten",
        "unterminated": "{term} niet beeindigd",
    },
    "pl": {
        "expected": "oczekiwano {term}",
        "invalid": "{term} nieprawidlowy",
        "unexpected": "{term} nieoczekiwany",
        "unknown": "{term} nieznany",
        "unsupported": "{term} nieobslugiwany",
        "duplicate": "{term} duplikat",
        "missing": "{term} brakujacy",
        "unclosed": "{term} niezamkniety",
        "unterminated": "{term} niezakonczony",
    },
    "ru": {
        "expected": "ozhidaetsya {term}",
        "invalid": "{term} nedopustimyi",
        "unexpected": "{term} neozhidannyi",
        "unknown": "{term} neizvestnyi",
        "unsupported": "{term} ne podderzhivaetsya",
        "duplicate": "{term} dublikat",
        "missing": "{term} otsutstvuet",
        "unclosed": "{term} ne zakryt",
        "unterminated": "{term} ne zavershen",
    },
    "uk": {
        "expected": "ochikuietsia {term}",
        "invalid": "{term} neprypustymyi",
        "unexpected": "{term} neochikuvanyi",
        "unknown": "{term} nevidomyi",
        "unsupported": "{term} ne pidtrymuietsia",
        "duplicate": "{term} dublikat",
        "missing": "{term} vidsutnii",
        "unclosed": "{term} ne zakryto",
        "unterminated": "{term} ne zaversheno",
    },
    "zh-CN": {
        "expected": "需要{term}",
        "invalid": "{term}无效",
        "unexpected": "{term}意外",
        "unknown": "未知{term}",
        "unsupported": "不支持{term}",
        "duplicate": "{term}重复",
        "missing": "缺少{term}",
        "unclosed": "{term}未闭合",
        "unterminated": "{term}未终止",
    },
    "ja": {
        "expected": "{term}が必要です",
        "invalid": "{term}が無効です",
        "unexpected": "予期しない{term}",
        "unknown": "不明な{term}",
        "unsupported": "{term}は未対応です",
        "duplicate": "{term}が重複しています",
        "missing": "{term}が欠落しています",
        "unclosed": "{term}が閉じられていません",
        "unterminated": "{term}が終端していません",
    },
    "ko": {
        "expected": "{term} 필요",
        "invalid": "잘못된 {term}",
        "unexpected": "예상치 못한 {term}",
        "unknown": "알 수 없는 {term}",
        "unsupported": "지원되지 않는 {term}",
        "duplicate": "중복된 {term}",
        "missing": "누락된 {term}",
        "unclosed": "닫히지 않은 {term}",
        "unterminated": "종결되지 않은 {term}",
    },
    "tr": {
        "expected": "{term} bekleniyor",
        "invalid": "gecersiz {term}",
        "unexpected": "beklenmeyen {term}",
        "unknown": "bilinmeyen {term}",
        "unsupported": "desteklenmeyen {term}",
        "duplicate": "yinelenen {term}",
        "missing": "eksik {term}",
        "unclosed": "kapatilmamis {term}",
        "unterminated": "sonlandirilmamis {term}",
    },
    "ar": {
        "expected": "{term} متوقع",
        "invalid": "{term} غير صالح",
        "unexpected": "{term} غير متوقع",
        "unknown": "{term} مجهول",
        "unsupported": "{term} غير مدعوم",
        "duplicate": "{term} مكرر",
        "missing": "{term} مفقود",
        "unclosed": "{term} غير مغلق",
        "unterminated": "{term} غير منته",
    },
}

RELATION_TEMPLATES: dict[str, dict[str, str]] = {
    "fr": {
        "requires_at_least_one": "{subject} necessite au moins un {object}",
        "cannot_have_body": "{subject} ne peut pas avoir de corps",
        "must_be": "{subject} doit etre {object}",
        "denied_by": "{subject} refuse par {object}",
        "forbids": "{subject} interdit {object}",
        "not_exported_by": "{subject} non exporte par {object}",
        "conflicts_with": "{subject} en conflit avec {object}",
        "references_unknown": "{subject} reference {object} inconnu",
        "deprecated": "{subject} est obsolete",
    },
    "es": {
        "requires_at_least_one": "{subject} requiere al menos un {object}",
        "cannot_have_body": "{subject} no puede tener cuerpo",
        "must_be": "{subject} debe ser {object}",
        "denied_by": "{subject} denegado por {object}",
        "forbids": "{subject} prohibe {object}",
        "not_exported_by": "{subject} no exportado por {object}",
        "conflicts_with": "{subject} entra en conflicto con {object}",
        "references_unknown": "{subject} referencia {object} desconocido",
        "deprecated": "{subject} esta obsoleto",
    },
    "de": {
        "requires_at_least_one": "{subject} erfordert mindestens ein {object}",
        "cannot_have_body": "{subject} darf keinen Body haben",
        "must_be": "{subject} muss {object} sein",
        "denied_by": "{subject} von {object} abgelehnt",
        "forbids": "{subject} verbietet {object}",
        "not_exported_by": "{subject} nicht von {object} exportiert",
        "conflicts_with": "{subject} kollidiert mit {object}",
        "references_unknown": "{subject} referenziert unbekanntes {object}",
        "deprecated": "{subject} ist veraltet",
    },
    "it": {
        "requires_at_least_one": "{subject} richiede almeno un {object}",
        "cannot_have_body": "{subject} non puo avere un corpo",
        "must_be": "{subject} deve essere {object}",
        "denied_by": "{subject} negato da {object}",
        "forbids": "{subject} vieta {object}",
        "not_exported_by": "{subject} non esportato da {object}",
        "conflicts_with": "{subject} e in conflitto con {object}",
        "references_unknown": "{subject} fa riferimento a {object} sconosciuto",
        "deprecated": "{subject} e deprecato",
    },
    "pt-BR": {
        "requires_at_least_one": "{subject} requer pelo menos um {object}",
        "cannot_have_body": "{subject} nao pode ter corpo",
        "must_be": "{subject} deve ser {object}",
        "denied_by": "{subject} negado por {object}",
        "forbids": "{subject} proibe {object}",
        "not_exported_by": "{subject} nao exportado por {object}",
        "conflicts_with": "{subject} conflita com {object}",
        "references_unknown": "{subject} referencia {object} desconhecido",
        "deprecated": "{subject} esta obsoleto",
    },
    "nl": {
        "requires_at_least_one": "{subject} vereist ten minste een {object}",
        "cannot_have_body": "{subject} mag geen body hebben",
        "must_be": "{subject} moet {object} zijn",
        "denied_by": "{subject} geweigerd door {object}",
        "forbids": "{subject} verbiedt {object}",
        "not_exported_by": "{subject} niet geexporteerd door {object}",
        "conflicts_with": "{subject} conflicteert met {object}",
        "references_unknown": "{subject} verwijst naar onbekend {object}",
        "deprecated": "{subject} is verouderd",
    },
    "pl": {
        "requires_at_least_one": "{subject} wymaga co najmniej jednego {object}",
        "cannot_have_body": "{subject} nie moze miec ciala",
        "must_be": "{subject} musi byc {object}",
        "denied_by": "{subject} odrzucony przez {object}",
        "forbids": "{subject} zabrania {object}",
        "not_exported_by": "{subject} nie eksportowany przez {object}",
        "conflicts_with": "{subject} koliduje z {object}",
        "references_unknown": "{subject} odwoluje sie do nieznanego {object}",
        "deprecated": "{subject} jest przestarzaly",
    },
    "ru": {
        "requires_at_least_one": "{subject} trebuet kak minimum odin {object}",
        "cannot_have_body": "{subject} ne mozhet imet telo",
        "must_be": "{subject} dolzhen byt {object}",
        "denied_by": "{subject} otklonen {object}",
        "forbids": "{subject} zapreshchaet {object}",
        "not_exported_by": "{subject} ne eksportirovan {object}",
        "conflicts_with": "{subject} konfliktuet s {object}",
        "references_unknown": "{subject} ssylkaetsya na neizvestnyi {object}",
        "deprecated": "{subject} ustarel",
    },
    "uk": {
        "requires_at_least_one": "{subject} vymahaie shchonaimenshe odyn {object}",
        "cannot_have_body": "{subject} ne mozhe maty tilo",
        "must_be": "{subject} maie buty {object}",
        "denied_by": "{subject} vidkhyleno {object}",
        "forbids": "{subject} zaboroniaie {object}",
        "not_exported_by": "{subject} ne eksportovano {object}",
        "conflicts_with": "{subject} konfliktuie z {object}",
        "references_unknown": "{subject} posylaietsia na nevidomyi {object}",
        "deprecated": "{subject} zastarilyi",
    },
    "zh-CN": {
        "requires_at_least_one": "{subject}至少需要一个{object}",
        "cannot_have_body": "{subject}不能有主体",
        "must_be": "{subject}必须是{object}",
        "denied_by": "{subject}被{object}拒绝",
        "forbids": "{subject}禁止{object}",
        "not_exported_by": "{subject}未由{object}导出",
        "conflicts_with": "{subject}与{object}冲突",
        "references_unknown": "{subject}引用未知{object}",
        "deprecated": "{subject}已弃用",
    },
    "ja": {
        "requires_at_least_one": "{subject}には少なくとも1つの{object}が必要です",
        "cannot_have_body": "{subject}は本体を持てません",
        "must_be": "{subject}は{object}である必要があります",
        "denied_by": "{subject}は{object}に拒否されました",
        "forbids": "{subject}は{object}を禁止します",
        "not_exported_by": "{subject}は{object}からエクスポートされていません",
        "conflicts_with": "{subject}は{object}と競合します",
        "references_unknown": "{subject}が不明な{object}を参照しています",
        "deprecated": "{subject}は非推奨です",
    },
    "ko": {
        "requires_at_least_one": "{subject}에는 최소 하나의 {object} 필요",
        "cannot_have_body": "{subject}는 본문을 가질 수 없음",
        "must_be": "{subject}는 {object}여야 함",
        "denied_by": "{subject}가 {object}에 의해 거부됨",
        "forbids": "{subject}가 {object} 금지",
        "not_exported_by": "{subject}가 {object}에서 내보내지지 않음",
        "conflicts_with": "{subject}가 {object}와 충돌",
        "references_unknown": "{subject}가 알 수 없는 {object} 참조",
        "deprecated": "{subject}는 폐기 예정",
    },
    "tr": {
        "requires_at_least_one": "{subject} en az bir {object} gerektirir",
        "cannot_have_body": "{subject} govde iceremez",
        "must_be": "{subject} {object} olmalidir",
        "denied_by": "{subject} {object} tarafindan reddedildi",
        "forbids": "{subject} {object} yasaklar",
        "not_exported_by": "{subject} {object} tarafindan disa aktarilmadi",
        "conflicts_with": "{subject} {object} ile cakisir",
        "references_unknown": "{subject} bilinmeyen {object} basvuruyor",
        "deprecated": "{subject} kullanımdan kaldirildi",
    },
    "ar": {
        "requires_at_least_one": "{subject} يتطلب {object} واحدا على الاقل",
        "cannot_have_body": "{subject} لا يمكن ان يملك جسما",
        "must_be": "{subject} يجب ان يكون {object}",
        "denied_by": "{subject} مرفوض بواسطة {object}",
        "forbids": "{subject} يمنع {object}",
        "not_exported_by": "{subject} غير مصدر بواسطة {object}",
        "conflicts_with": "{subject} يتعارض مع {object}",
        "references_unknown": "{subject} يشير الى {object} مجهول",
        "deprecated": "{subject} مهمل",
    },
}

PHRASE_TRANSLATIONS: dict[str, dict[str, str]] = {
    "fr": {
        "proc may exit without returning a value": "la procedure peut se terminer sans retourner de valeur",
        "forbidden top-level syntax in strict-core mode": "syntaxe de niveau superieur interdite en mode strict-core",
        "forbidden statement syntax in strict-core mode": "syntaxe d'instruction interdite en mode strict-core",
        "forbidden expression syntax in strict-core mode": "syntaxe d'expression interdite en mode strict-core",
        "expected 'case' or 'otherwise' in match": "'case' ou 'otherwise' attendu dans match",
        "expression is not callable": "l'expression n'est pas appelable",
        "parser made no progress": "le parseur ne progresse plus",
    },
    "es": {
        "proc may exit without returning a value": "proc puede salir sin devolver un valor",
        "forbidden top-level syntax in strict-core mode": "sintaxis de nivel superior prohibida en modo strict-core",
        "forbidden statement syntax in strict-core mode": "sintaxis de sentencia prohibida en modo strict-core",
        "forbidden expression syntax in strict-core mode": "sintaxis de expresion prohibida en modo strict-core",
        "expected 'case' or 'otherwise' in match": "se esperaba 'case' u 'otherwise' en match",
        "expression is not callable": "la expresion no se puede llamar",
        "parser made no progress": "el analizador no avanzo",
    },
    "de": {
        "proc may exit without returning a value": "proc kann enden, ohne einen Wert zurueckzugeben",
        "forbidden top-level syntax in strict-core mode": "verbotene Top-Level-Syntax im strict-core-Modus",
        "forbidden statement syntax in strict-core mode": "verbotene Anweisungssyntax im strict-core-Modus",
        "forbidden expression syntax in strict-core mode": "verbotene Ausdruckssyntax im strict-core-Modus",
        "expected 'case' or 'otherwise' in match": "'case' oder 'otherwise' in match erwartet",
        "expression is not callable": "Ausdruck ist nicht aufrufbar",
        "parser made no progress": "Parser hat keinen Fortschritt gemacht",
    },
    "it": {
        "proc may exit without returning a value": "proc puo terminare senza restituire un valore",
        "forbidden top-level syntax in strict-core mode": "sintassi di livello superiore vietata in modo strict-core",
        "forbidden statement syntax in strict-core mode": "sintassi di istruzione vietata in modo strict-core",
        "forbidden expression syntax in strict-core mode": "sintassi di espressione vietata in modo strict-core",
        "expected 'case' or 'otherwise' in match": "atteso 'case' o 'otherwise' in match",
        "expression is not callable": "l'espressione non e chiamabile",
        "parser made no progress": "il parser non e avanzato",
    },
    "pt-BR": {
        "proc may exit without returning a value": "proc pode terminar sem retornar um valor",
        "forbidden top-level syntax in strict-core mode": "sintaxe de nivel superior proibida no modo strict-core",
        "forbidden statement syntax in strict-core mode": "sintaxe de instrucao proibida no modo strict-core",
        "forbidden expression syntax in strict-core mode": "sintaxe de expressao proibida no modo strict-core",
        "expected 'case' or 'otherwise' in match": "esperado 'case' ou 'otherwise' em match",
        "expression is not callable": "a expressao nao pode ser chamada",
        "parser made no progress": "o parser nao avancou",
    },
    "zh-CN": {
        "proc may exit without returning a value": "proc 可能在未返回值的情况下退出",
        "forbidden top-level syntax in strict-core mode": "strict-core 模式禁止的顶层语法",
        "forbidden statement syntax in strict-core mode": "strict-core 模式禁止的语句语法",
        "forbidden expression syntax in strict-core mode": "strict-core 模式禁止的表达式语法",
        "expected 'case' or 'otherwise' in match": "match 中需要 'case' 或 'otherwise'",
        "expression is not callable": "表达式不可调用",
        "parser made no progress": "解析器没有取得进展",
    },
    "ja": {
        "proc may exit without returning a value": "proc が値を返さずに終了する可能性があります",
        "forbidden top-level syntax in strict-core mode": "strict-core モードで禁止されたトップレベル構文",
        "forbidden statement syntax in strict-core mode": "strict-core モードで禁止された文構文",
        "forbidden expression syntax in strict-core mode": "strict-core モードで禁止された式構文",
        "expected 'case' or 'otherwise' in match": "match には 'case' または 'otherwise' が必要です",
        "expression is not callable": "式は呼び出せません",
        "parser made no progress": "パーサが進行しませんでした",
    },
    "ko": {
        "proc may exit without returning a value": "proc가 값을 반환하지 않고 종료될 수 있음",
        "forbidden top-level syntax in strict-core mode": "strict-core 모드에서 금지된 최상위 구문",
        "forbidden statement syntax in strict-core mode": "strict-core 모드에서 금지된 문 구문",
        "forbidden expression syntax in strict-core mode": "strict-core 모드에서 금지된 식 구문",
        "expected 'case' or 'otherwise' in match": "match에서 'case' 또는 'otherwise' 필요",
        "expression is not callable": "식은 호출할 수 없음",
        "parser made no progress": "파서가 진행하지 못함",
    },
    "tr": {
        "proc may exit without returning a value": "proc deger dondurmeden cikabilir",
        "forbidden top-level syntax in strict-core mode": "strict-core modunda yasak ust duzey soz dizimi",
        "forbidden statement syntax in strict-core mode": "strict-core modunda yasak deyim soz dizimi",
        "forbidden expression syntax in strict-core mode": "strict-core modunda yasak ifade soz dizimi",
        "expected 'case' or 'otherwise' in match": "match icinde 'case' veya 'otherwise' bekleniyor",
        "expression is not callable": "ifade cagrilabilir degil",
        "parser made no progress": "ayristirici ilerlemedi",
    },
    "ar": {
        "proc may exit without returning a value": "قد يخرج proc دون ارجاع قيمة",
        "forbidden top-level syntax in strict-core mode": "بناء مستوى علوي محظور في وضع strict-core",
        "forbidden statement syntax in strict-core mode": "بناء عبارة محظور في وضع strict-core",
        "forbidden expression syntax in strict-core mode": "بناء تعبير محظور في وضع strict-core",
        "expected 'case' or 'otherwise' in match": "متوقع 'case' او 'otherwise' في match",
        "expression is not callable": "التعبير غير قابل للاستدعاء",
        "parser made no progress": "المحلل لم يتقدم",
    },
}

TECHNICAL_WORDS = {
    "ABI", "AST", "HIR", "IR", "MIR", "UTF", "Vitte", "bool", "case", "const", "entry",
    "end", "extern", "form", "give", "i32", "i64", "i128", "int", "match", "otherwise", "pick",
    "proc", "pull", "select", "space", "strict", "strict-core", "string", "u32", "u64",
    "u128", "use", "when",
}


def parse_ftl(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def translate_terms(locale: str, text: str) -> str:
    terms = TERMS.get(locale)
    if not terms:
        return text

    def replace(match: re.Match[str]) -> str:
        word = match.group(0)
        if word in TECHNICAL_WORDS or word.upper() == word:
            return word
        return terms.get(word.lower(), word)

    return re.sub(r"[A-Za-z][A-Za-z0-9_-]*", replace, text)


def translate_message(locale: str, message: str) -> str:
    if locale == "en":
        return message
    normalized = " ".join(message.split())
    exact = PHRASE_TRANSLATIONS.get(locale, {}).get(normalized)
    if exact:
        return exact

    templates = MODIFIER_TEMPLATES.get(locale, {})
    for modifier, template in templates.items():
        prefix = f"{modifier} "
        if normalized.startswith(prefix):
            term = translate_terms(locale, normalized[len(prefix):])
            return template.format(term=term)

    relations = RELATION_TEMPLATES.get(locale, {})
    relation_patterns: tuple[tuple[str, str], ...] = (
        (r"(.+) requires at least one (.+)", "requires_at_least_one"),
        (r"(.+) cannot have a body", "cannot_have_body"),
        (r"(.+) must be (.+)", "must_be"),
        (r"(.+) denied by (.+)", "denied_by"),
        (r"(.+) forbids (.+)", "forbids"),
        (r"(.+) not exported by (.+)", "not_exported_by"),
        (r"(.+) conflicts with (.+)", "conflicts_with"),
        (r"(.+) references unknown (.+)", "references_unknown"),
        (r"(.+) is deprecated", "deprecated"),
    )
    for pattern, key in relation_patterns:
        match = re.fullmatch(pattern, normalized)
        if match and key in relations:
            groups = match.groups()
            return relations[key].format(
                subject=translate_terms(locale, groups[0]),
                object=translate_terms(locale, groups[1]) if len(groups) > 1 else "",
            )

    for suffix, modifier in (
        (" not found", "missing"),
        (" too large", "too"),
        (" mismatch", "mismatch"),
        (" conflict", "conflict"),
        (" failed", "failed"),
    ):
        if normalized.endswith(suffix):
            stem = translate_terms(locale, normalized[: -len(suffix)])
            word = TERMS.get(locale, {}).get(modifier, modifier)
            if locale in ("zh-CN", "ja", "ko"):
                return f"{stem}{word}"
            return f"{stem} {word}"

    translated = translate_terms(locale, normalized)
    translated = re.sub(r"\s+", " ", translated).strip()
    return translated or message


def translated_value(locale: str, code: str, fallback: dict[str, str]) -> str:
    message = fallback.get(code, readable_from_code(code))
    return translate_message(locale, message)


def render_locale(locale: str, codes: list[str], existing: dict[str, str], fallback: dict[str, str]) -> str:
    title = locale_by_code(locale).name
    header = LOCALE_HEADERS.get(locale, LOCALE_HEADERS["en"])[0]
    lines = [f"# Vitte diagnostics ({title})", "#", f"# {header}", ""]
    for code in codes:
        if locale == "en":
            value = existing.get(code, fallback.get(code, readable_from_code(code)))
        else:
            value = translated_value(locale, code, fallback)
        lines.append(f"{code} = {value}")
    return "\n".join(lines) + "\n"


def missing_codes(codes: list[str], existing: dict[str, str]) -> list[str]:
    return [code for code in codes if code not in existing]


def append_missing_codes(current: str, missing: list[str], fallback: dict[str, str]) -> str:
    if not missing:
        return current
    body = current
    if body and not body.endswith("\n"):
        body += "\n"
    if body and not body.endswith("\n\n"):
        body += "\n"
    body += "# Added from tests/diag_snapshots/core_diagnostic_codes.txt\n"
    for code in missing:
        body += f"{code} = {fallback.get(code, readable_from_code(code))}\n"
    return body


def render_explain_locale(locale: str, codes: list[str], messages: dict[str, str], existing: dict[str, str], fallback: dict[str, str]) -> str:
    title = locale_by_code(locale).name
    header = LOCALE_HEADERS.get(locale, LOCALE_HEADERS["en"])[1]
    lines = [
        f"# Vitte diagnostic explanations ({title})",
        "#",
        f"# {header}",
        "",
    ]
    for code in codes:
        fields = explanation_fields(code, messages.get(code))
        for suffix in ("summary", "cause", "step1", "fix", "example"):
            key = f"{code}.{suffix}"
            value = existing.get(key, fallback.get(key, fields[suffix]))
            if value in (
                "The general phase found code that violates this diagnostic rule.",
                "Fix the first span reported for this diagnostic, then run the command again.",
                "Follow the primary help text and make the smallest source change that removes the first error.",
            ):
                value = fields[suffix]
            if locale != "en" and suffix == "summary":
                value = translate_message(locale, value.removesuffix(".")) + "."
            lines.append(f"{key} = {value}")
        lines.append("")
    return "\n".join(lines)


def sync_locale(locale: str, check: bool) -> int:
    selected_codes = [
        line.strip()
        for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]
    codes = public_diagnostic_codes(selected_codes)
    out_path = LOCALES_DIR / locale / "diagnostics.ftl"
    fallback = parse_ftl(LOCALES_DIR / "en" / "diagnostics.ftl")
    existing = parse_ftl(out_path)
    missing = missing_codes(codes, existing)
    if check:
        if not out_path.exists():
            print(f"[diagnostics-ftl][error] missing {out_path.relative_to(ROOT)}")
            return 1
        if missing:
            print(f"[diagnostics-ftl][error] stale {out_path.relative_to(ROOT)}")
            for code in missing:
                print(f"- missing code key {code}")
            return 1
        return 0
    out_path.parent.mkdir(parents=True, exist_ok=True)
    if out_path.exists():
        current = out_path.read_text(encoding="utf-8")
    else:
        current = ""
    if locale == "en":
        updated = append_missing_codes(current, missing, fallback) if current else render_locale(locale, codes, existing, fallback)
    else:
        updated = render_locale(locale, codes, existing, fallback)
    if updated != current:
        out_path.write_text(updated, encoding="utf-8")
        print(f"[diagnostics-ftl] wrote {out_path.relative_to(ROOT)}")
    else:
        print(f"[diagnostics-ftl] ok {out_path.relative_to(ROOT)}")
    explain_path = LOCALES_DIR / locale / "diagnostics_explain.ftl"
    explain_existing = parse_ftl(explain_path)
    explain_fallback = parse_ftl(LOCALES_DIR / "en" / "diagnostics_explain.ftl")
    explain_updated = render_explain_locale(locale, codes, parse_ftl(out_path), explain_existing, explain_fallback)
    if check:
        explain_missing = [
            f"{code}.{suffix}"
            for code in codes
            for suffix in ("summary", "cause", "step1", "fix", "example")
            if f"{code}.{suffix}" not in explain_existing
        ]
        if not explain_path.exists():
            print(f"[diagnostics-ftl][error] missing {explain_path.relative_to(ROOT)}")
            return 1
        if explain_missing:
            print(f"[diagnostics-ftl][error] stale {explain_path.relative_to(ROOT)}")
            for key in explain_missing[:40]:
                print(f"- missing explain key {key}")
            if len(explain_missing) > 40:
                print(f"- ... {len(explain_missing) - 40} more")
            return 1
        return 0
    if explain_path.exists():
        explain_current = explain_path.read_text(encoding="utf-8")
    else:
        explain_current = ""
    if explain_updated != explain_current:
        explain_path.write_text(explain_updated, encoding="utf-8")
        print(f"[diagnostics-ftl] wrote {explain_path.relative_to(ROOT)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    return max(sync_locale(locale, args.check) for locale in supported_locale_codes())


if __name__ == "__main__":
    raise SystemExit(main())
