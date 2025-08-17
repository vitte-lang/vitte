;; vitte-mode — major mode minimal pour Vitte
;; -*- lexical-binding: t; -*-
(defvar vitte-mode-hook nil)
(defvar vitte-mode-map (make-sparse-keymap))

(defvar vitte-font-lock-keywords
  '(("\<\(fn\|let\|mut\|const\|if\|else\|while\|for\|return\|match\|struct\|enum\|impl\|use\|mod\|pub\|async\|await\)\>" . font-lock-keyword-face)
    ("\<[0-9]+\(\.[0-9]+\)?\>" . font-lock-constant-face)
    ("//.*$" . font-lock-comment-face)
    (""\\(?:\\.|[^\\"]\)*"" . font-lock-string-face)))

(defun vitte-indent-line ()
  (interactive)
  (let ((savep (> (current-column) (current-indentation)))
        (indent (condition-case nil (max (vitte-calc-indent) 0) (error 0))))
    (if savep (save-excursion (indent-line-to indent)) (indent-line-to indent))))

(defun vitte-calc-indent () 0)

(define-derived-mode vitte-mode prog-mode "Vitte"
  "Major mode for the Vitte language."
  (setq-local font-lock-defaults '(vitte-font-lock-keywords))
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local indent-line-function 'vitte-indent-line))

(provide 'vitte-mode)
