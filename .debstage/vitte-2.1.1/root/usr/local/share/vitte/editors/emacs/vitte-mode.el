;;; vitte-mode.el --- Major mode for Vitte -*- lexical-binding: t; -*-

;; Minimal Vitte mode for syntax highlighting

(defvar vitte-mode-hook nil)

(defgroup vitte nil
  "Vitte language support."
  :group 'languages)

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

(defun vitte--completion-at-point ()
  "Provide basic keyword/type/builtin completion at point."
  (let ((bounds (bounds-of-thing-at-point 'symbol)))
    (when bounds
      (list (car bounds)
            (cdr bounds)
            (append vitte-keywords vitte-types vitte-builtins)
            :exclusive 'no))))

(defun vitte--file-or-error ()
  "Return current buffer file path or raise an error."
  (or (buffer-file-name)
      (user-error "Buffer is not visiting a file")))

(defun vitte--run (subcommand file)
  "Run Vitte SUBCOMMAND on FILE using `compile'."
  (let ((cmd (format "%s %s %s"
                     (shell-quote-argument vitte-binary)
                     subcommand
                     (shell-quote-argument file))))
    (compile cmd)))

;;;###autoload
(defun vitte-check-buffer ()
  "Run `vitte check` on current buffer file."
  (interactive)
  (vitte--run "check" (vitte--file-or-error)))

;;;###autoload
(defun vitte-build-buffer ()
  "Run `vitte build` on current buffer file."
  (interactive)
  (vitte--run "build" (vitte--file-or-error)))

;;;###autoload
(defun vitte-parse-buffer ()
  "Run `vitte parse` on current buffer file."
  (interactive)
  (vitte--run "parse" (vitte--file-or-error)))

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
  (add-hook 'completion-at-point-functions #'vitte--completion-at-point nil t)
  (when (require 'vitte-indent nil t)
    (setq-local indent-line-function #'vitte-indent-line)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))

(provide 'vitte-mode)
;;; vitte-mode.el ends here
