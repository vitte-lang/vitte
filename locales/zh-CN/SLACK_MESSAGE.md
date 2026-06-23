# Slack message (copy into your channel)

Canal: `#translations` (ou votre canal d'équipe de traduction)

Message (anglais/français/中文):

---
EN:
Hello everyone — a PR for reviewing Simplified Chinese translations of Vitte diagnostics has been opened:
https://github.com/vitte-lang/vitte/pull/106

Files to review:
- locales/zh-CN/diagnostics.ftl
- locales/zh-CN/diagnostics_explain.ftl

Instructions: see `locales/zh-CN/TRANSLATORS.md` in the repo. To validate locally run:
`python3 tools/check_diagnostics_locales.py` (should print `[diagnostics-locales] OK`).

FR:
Bonjour — une PR de relecture des traductions zh-CN a été ouverte :
https://github.com/vitte-lang/vitte/pull/106

Fichiers : `locales/zh-CN/diagnostics.ftl`, `locales/zh-CN/diagnostics_explain.ftl`.
Voir `locales/zh-CN/TRANSLATORS.md` pour instructions. Pour valider localement exécutez :
`python3 tools/check_diagnostics_locales.py` (doit afficher `[diagnostics-locales] OK`).

ZH:
大家好 — 我已打开一条用于审校 Vitte 诊断信息（简体中文）翻译的 PR：
https://github.com/vitte-lang/vitte/pull/106

需要审校的文件：
- locales/zh-CN/diagnostics.ftl
- locales/zh-CN/diagnostics_explain.ftl

审校说明见 `locales/zh-CN/TRANSLATORS.md`。本地校验命令：
`python3 tools/check_diagnostics_locales.py`（应输出 `[diagnostics-locales] OK`）。

---

Please paste into your translation Slack/Teams channel and ping the translators.
