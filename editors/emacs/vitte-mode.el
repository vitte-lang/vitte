;;; vitte-mode.el --- Major mode for Vitte -*- lexical-binding: t; -*-

;; Minimal Vitte mode for syntax highlighting

(defvar vitte-mode-hook nil)

(defvar vitte-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?_ "w" st)
    (modify-syntax-entry ?/ ". 124b" st)
    (modify-syntax-entry ?* ". 23" st)
    (modify-syntax-entry ?\n ">" st)
    st)
  "Syntax table for `vitte-mode'.")

(defconst vitte-keywords
  '("space" "pull" "use" "share" "const" "let" "make" "type" "form" "trait"
    "pick" "case" "proc" "entry" "at" "macro" "give" "emit" "asm" "unsafe"
    "if" "else" "otherwise" "loop" "for" "in" "break" "continue" "select"
    "when" "match" "return" "not" "and" "or" "is" "as"))

(defconst vitte-types
  '("bool" "string" "int" "char" "i8" "i16" "i32" "i64" "isize" "u8" "u16"
    "u32" "u64" "usize" "f32" "f64"))

(defconst vitte-builtins
  '("true" "false" "null"))

(defvar vitte-font-lock-keywords
  `((,(regexp-opt vitte-keywords 'symbols) . font-lock-keyword-face)
    (,(regexp-opt vitte-types 'symbols) . font-lock-type-face)
    (,(regexp-opt vitte-builtins 'symbols) . font-lock-constant-face)
    ("\\<\\(TODO\\|FIXME\\|NOTE\\|XXX\\)\\>" . font-lock-warning-face)
    ("#\\[[^]]+\\]" . font-lock-preprocessor-face)
    ("\\<use\\>\\s-+\\([A-Za-z0-9_./]+\\)" 1 font-lock-constant-face)))

;;;###autoload
(define-derived-mode vitte-mode prog-mode "Vitte"
  "Major mode for editing Vitte source files."
  :syntax-table vitte-mode-syntax-table
  (setq font-lock-defaults '(vitte-font-lock-keywords))
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "//+\\s-*")
  (setq-local comment-use-syntax t)
  (when (require 'vitte-indent nil t)
    (setq-local indent-line-function #'vitte-indent-line)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))

(provide 'vitte-mode)
;;; vitte-mode.el ends here
