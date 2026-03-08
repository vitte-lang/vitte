;;; vitte-mode.el --- Major mode for Vitte (generated) -*- lexical-binding: t; -*-

(defgroup vitte nil "Vitte language support." :group 'languages)

(defcustom vitte-binary
  (or (executable-find "vitte") "vitte")
  "Path to the Vitte binary used by helper commands."
  :type 'string
  :group 'vitte)

(defvar vitte-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map (kbd "C-c C-c") #'vitte-check-buffer)
    (define-key map (kbd "C-c C-b") #'vitte-build-buffer)
    (define-key map (kbd "C-c C-p") #'vitte-parse-buffer)
    map)
  "Keymap for `vitte-mode'.")

(defvar vitte-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?_ "w" st)
    (modify-syntax-entry ?/ ". 124b" st)
    (modify-syntax-entry ?* ". 23" st)
    (modify-syntax-entry ?\n ">" st)
    (modify-syntax-entry ?\" "\"" st)
    st)
  "Syntax table for `vitte-mode'.")

(defface vitte-diag-face
  '((t :inherit font-lock-warning-face :weight bold))
  "Face for VITTE diagnostics codes.")

(defface vitte-alias-face
  '((t :inherit font-lock-variable-name-face :slant italic))
  "Face for *_pkg aliases.")

(defface vitte-contract-face
  '((t :inherit font-lock-preprocessor-face :weight bold))
  "Face for ROLE-CONTRACT headers.")

(defconst vitte-keywords '("space" "pull" "use" "share" "const" "let" "make" "type" "form" "trait" "pick" "case" "proc" "entry" "at" "macro" "give" "emit" "asm" "unsafe" "if" "else" "otherwise" "loop" "for" "in" "break" "continue" "select" "when" "match" "return" "not" "and" "or" "is" "as"))
(defconst vitte-decls '("proc" "form" "pick" "trait" "entry"))
(defconst vitte-types '("bool" "string" "int" "char" "i8" "i16" "i32" "i64" "isize" "u8" "u16" "u32" "u64" "usize" "f32" "f64"))
(defconst vitte-builtins '("true" "false" "null"))

(defvar vitte-font-lock-keywords
  `((,(regexp-opt vitte-keywords 'symbols) . font-lock-keyword-face)
    (,(regexp-opt vitte-decls 'symbols) . font-lock-function-name-face)
    (,(regexp-opt vitte-types 'symbols) . font-lock-type-face)
    (,(regexp-opt vitte-builtins 'symbols) . font-lock-constant-face)
    ("\\<\\(proc\\|form\\|pick\\|trait\\|entry\\)\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)" 2 font-lock-function-name-face)
    ("\\<\\(space\\|pull\\|share\\|at\\)\\>\\s-+\\([A-Za-z0-9_./]+\\)" 2 font-lock-constant-face)
    ("\\<[A-Za-z_][A-Za-z0-9_]*\\s-*(" . font-lock-function-name-face)
    ("'\\([^'\\\\]\\|\\\\.\\)'" . font-lock-string-face)
    ("\\<[A-Za-z_][A-Za-z0-9_]*_pkg\\>" . 'vitte-alias-face)
    ("\\<VITTE-[A-Z]+[0-9]\\{4\\}\\>" . 'vitte-diag-face)
    ("^\\s-*<<<\\s-+ROLE-CONTRACT" . 'vitte-contract-face)
    ("\\<\\(diagnostics_[A-Za-z0-9_]*\\|quickfix_[A-Za-z0-9_]*\\|doctor_[A-Za-z0-9_]*\\)\\>" . font-lock-builtin-face)
    ("\\<\\(TODO|FIXME|NOTE|XXX\\)\\>" . font-lock-warning-face)
    ("\\<use\\>\\s-+\\([A-Za-z0-9_./]+\\)" 1 font-lock-constant-face)))

(defvar vitte-imenu-generic-expression
  '(("proc" "^\\s-*proc\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("form" "^\\s-*form\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("pick" "^\\s-*pick\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("trait" "^\\s-*trait\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("entry" "^\\s-*entry\\s-+\\([A-Za-z0-9_]+\\)" 1)))

(defun vitte--completion-at-point ()
  (let ((bounds (bounds-of-thing-at-point 'symbol)))
    (when bounds
      (list (car bounds)
            (cdr bounds)
            (append vitte-keywords vitte-decls vitte-types vitte-builtins)
            :exclusive 'no))))

(defun vitte--file-or-error ()
  (or (buffer-file-name) (user-error "Buffer is not visiting a file")))

(defun vitte--run (subcommand file)
  (compile (format "%s %s %s"
                   (shell-quote-argument vitte-binary)
                   subcommand
                   (shell-quote-argument file))))

(defun vitte-check-buffer () (interactive) (vitte--run "check" (vitte--file-or-error)))
(defun vitte-build-buffer () (interactive) (vitte--run "build" (vitte--file-or-error)))
(defun vitte-parse-buffer () (interactive) (vitte--run "parse" (vitte--file-or-error)))

;;;###autoload
(define-derived-mode vitte-mode prog-mode "Vitte"
  "Major mode for editing Vitte source files."
  :keymap vitte-mode-map
  :syntax-table vitte-mode-syntax-table
  (setq font-lock-defaults '(vitte-font-lock-keywords))
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "//+\\s-*")
  (setq-local comment-use-syntax t)
  (setq-local imenu-generic-expression vitte-imenu-generic-expression)
  (add-hook 'completion-at-point-functions #'vitte--completion-at-point nil t)
  (when (require 'vitte-indent nil t)
    (setq-local indent-line-function #'vitte-indent-line))
  (when (require 'lsp-mode nil t)
    (add-hook 'vitte-mode-hook #'lsp-deferred nil t)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))

(provide 'vitte-mode)
;;; vitte-mode.el ends here
