;;; vitte-mode.el --- Major mode for Vitte (generated) -*- lexical-binding: t; -*-

(defgroup vitte nil "Vitte language support." :group 'languages)

(declare-function vitte-indent-line "vitte-indent")

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

(defface vitte-alias-face
  '((t :inherit font-lock-variable-name-face :slant italic))
  "Face for *_pkg aliases.")

(defface vitte-property-face
  '((t :inherit font-lock-variable-name-face))
  "Face for property/member names.")

(defconst vitte-keywords '("space" "use" "export" "const" "static" "type" "opaque" "form" "pick" "trait" "impl" "proc" "extern" "intrinsic" "macro" "test" "pub" "async" "unsafe" "comptime" "where" "requires" "ensures" "let" "mut" "set" "return" "give" "break" "continue" "defer" "assert" "if" "else" "while" "loop" "for" "in" "match" "asm" "ref" "move" "await" "not" "and" "or" "self" "as"))
(defconst vitte-decls '("space" "use" "export" "const" "static" "type" "opaque" "form" "pick" "trait" "impl" "proc" "extern" "intrinsic" "macro" "test"))
(defconst vitte-types '("void" "never" "unit" "bool" "char" "rune" "str" "string" "bytes" "cstr" "int" "i8" "i16" "i32" "i64" "i128" "isize" "u8" "u16" "u32" "u64" "u128" "usize" "intptr" "uintptr" "f16" "f32" "f64" "f128" "c_char" "c_int" "c_uint" "c_long" "c_ulong" "c_void"))
(defconst vitte-builtins '("true" "false" "null"))

(defvar vitte-font-lock-keywords
  `((,(regexp-opt vitte-keywords 'symbols) . font-lock-keyword-face)
    (,(regexp-opt vitte-types 'symbols) . font-lock-type-face)
    (,(regexp-opt vitte-builtins 'symbols) . font-lock-constant-face)
    ("\\<\\(proc\\|intrinsic\\|macro\\|test\\)\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)" 2 font-lock-function-name-face)
    ("\\<\\(form\\|pick\\|trait\\|type\\|opaque\\)\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)" 2 font-lock-type-face)
    ("\\<\\(space\\|use\\)\\>\\s-+\\([A-Za-z0-9_/:]+\\)" 2 font-lock-constant-face)
    ("\\<export\\>\\s-+\\(\\*\\|[A-Za-z0-9_./:]+\\)" 1 font-lock-constant-face)
    ("\\<as\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\(?:_pkg\\)?\\)" 1 'vitte-alias-face)
    ("\\<\\(let\\|const\\|static\\)\\>\\s-+\\(?:mut\\s-+\\)?\\([A-Za-z_][A-Za-z0-9_]*\\)" 2 font-lock-variable-name-face)
    ("\\<set\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)" 1 font-lock-variable-name-face)
    ("\\<for\\>\\s-+\\([A-Za-z_][A-Za-z0-9_]*\\)\\s-+in\\>" 1 font-lock-variable-name-face)
    ("[(,]\\s-*\\([A-Za-z_][A-Za-z0-9_]*\\)\\s-*[: ,)]" 1 font-lock-variable-name-face)
    ("\\<\\([A-Za-z_][A-Za-z0-9_]*\\(?:/[A-Za-z_][A-Za-z0-9_]*\\)?\\)\\s-*(" 1 font-lock-function-name-face)
    ("\\<\\([A-Za-z_][A-Za-z0-9_]*\\(?:\\.[A-Za-z_][A-Za-z0-9_]*\\)+\\)\\s-*(" 1 font-lock-function-name-face)
    ("\\<::\\([A-Z][A-Za-z0-9_]*\\)\\>" 1 font-lock-constant-face)
    ("\\.[[:space:]]*\\([A-Za-z_][A-Za-z0-9_]*\\)" 1 'vitte-property-face)
    ("'\\([^'\\\\]\\|\\\\.\\)'" . font-lock-string-face)
    ("\\<[A-Za-z_][A-Za-z0-9_]*_pkg\\>" . 'vitte-alias-face)
    ("\\<\\(TODO\\|FIXME\\|BUG\\|HACK\\|NOTE\\|XXX\\)\\>" . font-lock-warning-face)
    ("\\<use\\>\\s-+\\([A-Za-z0-9_./:]+\\)" 1 font-lock-constant-face)
    ("\\<\\(void\\|never\\|unit\\|bool\\|char\\|rune\\|str\\|string\\|bytes\\|cstr\\|int\\|i8\\|i16\\|i32\\|i64\\|i128\\|isize\\|u8\\|u16\\|u32\\|u64\\|u128\\|usize\\|intptr\\|uintptr\\|f16\\|f32\\|f64\\|f128\\|c_char\\|c_int\\|c_uint\\|c_long\\|c_ulong\\|c_void\\)\\>" . font-lock-type-face)
    ("\\<\\(true\\|false\\|null\\)\\>" . font-lock-constant-face)))

(defvar vitte-imenu-generic-expression
  '(("proc" "^\\s-*proc\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("macro" "^\\s-*macro\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("type" "^\\s-*\\(?:form\\|pick\\|trait\\|type\\|opaque\\)\\s-+\\([A-Za-z0-9_]+\\)" 1)
    ("test" "^\\s-*test\\s-+\\([A-Za-z0-9_]+\\)" 1)))

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
    (setq-local indent-line-function #'vitte-indent-line)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.\\(vit\\|vitte\\|vitl\\)\\'" . vitte-mode))

(provide 'vitte-mode)
;;; vitte-mode.el ends here
