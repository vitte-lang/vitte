;;; vitte-mode.el --- Advanced major mode for Vitte -*- lexical-binding: t; -*-

;; Modern Vitte major mode:
;; - syntax highlighting
;; - indentation integration
;; - imenu
;; - compilation
;; - completion
;; - xref-ready structure
;; - electric pairs
;; - syntax-aware navigation
;; - modern compiler integration

(require 'compile)
(require 'imenu)
(require 'prog-mode)
(require 'cl-lib)

(defgroup vitte nil
  "Major mode for the Vitte language."
  :group 'languages
  :prefix "vitte-")

(defcustom vitte-binary
  (or (executable-find "vitte") "vitte")
  "Path to the Vitte compiler executable."
  :type 'string
  :group 'vitte)

(defcustom vitte-format-on-save nil
  "Automatically format Vitte buffers on save."
  :type 'boolean
  :group 'vitte)

(defcustom vitte-indent-offset 4
  "Indent width."
  :type 'integer
  :group 'vitte)

;;;; ------------------------------------------------------------------
;;;; Faces
;;;; ------------------------------------------------------------------

(defface vitte-diagnostic-face
  '((t :inherit font-lock-warning-face :weight bold))
  "Face for diagnostics.")

(defface vitte-attribute-face
  '((t :inherit font-lock-preprocessor-face))
  "Face for attributes.")

(defface vitte-contract-face
  '((t :inherit font-lock-preprocessor-face :weight bold))
  "Face for ROLE-CONTRACT sections.")

(defface vitte-module-face
  '((t :inherit font-lock-constant-face))
  "Face for module paths.")

(defface vitte-builtin-face
  '((t :inherit font-lock-builtin-face))
  "Face for builtins.")

(defface vitte-lifetime-face
  '((t :inherit font-lock-variable-name-face :slant italic))
  "Face for lifetime/region style identifiers.")

;;;; ------------------------------------------------------------------
;;;; Syntax table
;;;; ------------------------------------------------------------------

(defvar vitte-mode-syntax-table
  (let ((st (make-syntax-table)))

    ;; identifiers
    (modify-syntax-entry ?_ "w" st)

    ;; comments
    (modify-syntax-entry ?/ ". 124b" st)
    (modify-syntax-entry ?* ". 23" st)
    (modify-syntax-entry ?\n ">" st)

    ;; strings
    (modify-syntax-entry ?\" "\"" st)
    (modify-syntax-entry ?' "\"" st)

    ;; punctuation
    (modify-syntax-entry ?< "(>" st)
    (modify-syntax-entry ?> ")<" st)

    st)
  "Syntax table for Vitte.")

;;;; ------------------------------------------------------------------
;;;; Keywords
;;;; ------------------------------------------------------------------

(defconst vitte-keywords
  '(
    "space" "pull" "from" "use" "share" "export" "import" "all"

    "proc" "flow" "entry"
    "form" "class" "pick" "trait"
    "impl" "enum" "union" "interface"
    "type"

    "let" "make" "keep" "set"
    "const" "static" "global"

    "if" "elif" "else"
    "when" "otherwise"
    "match" "select" "case"

    "loop" "while" "until"
    "for" "each" "in"

    "break" "continue" "retry"

    "give" "return" "emit" "yield"

    "try" "catch" "finally"
    "panic" "assert"

    "unsafe" "asm"
    "foreign" "extern"

    "async" "await" "defer"

    "is" "as"
    "and" "or" "xor" "not"

    "move" "borrow" "ref"
    "self" "super"

    "true" "false"
    "null" "none"
    ))

(defconst vitte-types
  '(
    "void"
    "bool"

    "i8" "i16" "i32" "i64" "i128" "isize"
    "u8" "u16" "u32" "u64" "u128" "usize"

    "f16" "f32" "f64"

    "int" "uint"
    "float"

    "string"
    "str"
    "char"
    "rune"

    "vec"
    "map"
    "list"
    "set"
    "tuple"

    "option"
    "result"
    ))

(defconst vitte-builtins
  '(
    "Some"
    "None"
    "Ok"
    "Err"

    "Success"
    "Failure"
    ))

(defconst vitte-declaration-keywords
  '(
    "proc"
    "flow"
    "entry"
    "form"
    "class"
    "pick"
    "trait"
    "impl"
    "enum"
    "union"
    "interface"
    "type"
    ))

;;;; ------------------------------------------------------------------
;;;; Regex helpers
;;;; ------------------------------------------------------------------

(defconst vitte--identifier-re
  "[A-Za-z_][A-Za-z0-9_]*")

(defconst vitte--module-path-re
  "[A-Za-z0-9_./:]+")

(defconst vitte--qualified-ident-re
  "[A-Za-z0-9_./:]+")

;;;; ------------------------------------------------------------------
;;;; Font lock
;;;; ------------------------------------------------------------------

(defvar vitte-font-lock-keywords
  `(

    ;; keywords
    (,(regexp-opt vitte-keywords 'symbols)
     . font-lock-keyword-face)

    ;; primitive types
    (,(regexp-opt vitte-types 'symbols)
     . font-lock-type-face)

    ;; builtins
    (,(regexp-opt vitte-builtins 'symbols)
     . vitte-builtin-face)

    ;; declarations
    (,(concat
       "\\_<"
       (regexp-opt vitte-declaration-keywords t)
       "\\_>\\s-+\$begin:math:text$\"
       vitte\-\-identifier\-re
       \"\\$end:math:text$")
     1 font-lock-function-name-face)

    ;; module paths
    ("\\_<\$begin:math:text$space\\\\\|pull\\\\\|from\\\\\|use\\\\\|share\\\\\|import\\\\\|export\\\\\|at\\$end:math:text$\\_>\\s-+\$begin:math:text$\[A\-Za\-z0\-9\_\.\/\:\]\+\\$end:math:text$"
     2 'vitte-module-face)

    ;; aliases
    ("\\_<as\\_>\\s-+\$begin:math:text$\[A\-Za\-z\_\]\[A\-Za\-z0\-9\_\]\*\\$end:math:text$"
     1 font-lock-variable-name-face)

    ;; attributes
    ("#\$begin:math:display$\[\^\]\]\+\\$end:math:display$"
     . 'vitte-attribute-face)

    ;; diagnostics
    ("\\_<VITTE-[A-Z]+[0-9]\\{4\\}\\_>"
     . 'vitte-diagnostic-face)

    ;; compiler helpers
    ("\\_<\$begin:math:text$diagnostics\_\[A\-Za\-z0\-9\_\]\*\\\\\|quickfix\_\[A\-Za\-z0\-9\_\]\*\\\\\|doctor\_\[A\-Za\-z0\-9\_\]\*\\$end:math:text$\\_>"
     . font-lock-builtin-face)

    ;; contracts
    ("^\\s-*<<<\\s-*ROLE-[A-Z-]+"
     . 'vitte-contract-face)

    ;; TODO
    ("\\_<\$begin:math:text$TODO\\\\\|FIXME\\\\\|BUG\\\\\|HACK\\\\\|NOTE\\\\\|XXX\\\\\|SAFETY\\\\\|PERF\\$end:math:text$\\_>"
     . font-lock-warning-face)

    ;; constants
    ("\\_<[A-Z][A-Z0-9_]+\\_>"
     . font-lock-constant-face)

    ;; lifetimes / regions
    ("\\_<\$begin:math:text$region\\\\\|lifetime\\\\\|loan\\\\\|borrow\\\\\|ownership\\$end:math:text$\\_>"
     . 'vitte-lifetime-face)

    ;; char literals
    ("'\$begin:math:text$\[\^\'\\\\\\\\\]\\\\\|\\\\\\\\\.\\$end:math:text$'"
     . font-lock-string-face)

    ;; raw strings
    ("r\"[^\"]*\""
     . font-lock-string-face)

    ;; numeric suffixes
    ("\\_<[0-9][0-9_]*\$begin:math:text$i8\\\\\|i16\\\\\|i32\\\\\|i64\\\\\|u8\\\\\|u16\\\\\|u32\\\\\|u64\\\\\|f32\\\\\|f64\\$end:math:text$\\_>"
     . font-lock-constant-face)

    ))

;;;; ------------------------------------------------------------------
;;;; Imenu
;;;; ------------------------------------------------------------------

(defvar vitte-imenu-generic-expression
  `(
    ("Procedures"
     ,(concat "^\\s-*proc\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Flows"
     ,(concat "^\\s-*flow\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Forms"
     ,(concat "^\\s-*form\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Classes"
     ,(concat "^\\s-*class\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Traits"
     ,(concat "^\\s-*trait\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Enums"
     ,(concat "^\\s-*pick\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Entries"
     ,(concat "^\\s-*entry\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ("Types"
     ,(concat "^\\s-*type\\s-+\\(" vitte--identifier-re "\\)")
     1)

    ))

;;;; ------------------------------------------------------------------
;;;; Completion
;;;; ------------------------------------------------------------------

(defun vitte-completion-at-point ()
  "Completion backend for Vitte."

  (let ((bounds (bounds-of-thing-at-point 'symbol)))

    (when bounds
      (list
       (car bounds)
       (cdr bounds)

       (delete-dups
        (append
         vitte-keywords
         vitte-types
         vitte-builtins))

       :exclusive 'no))))

;;;; ------------------------------------------------------------------
;;;; Compilation integration
;;;; ------------------------------------------------------------------

(defun vitte--buffer-file ()
  (or
   (buffer-file-name)
   (user-error "Buffer is not visiting a file")))

(defun vitte--project-root ()
  (or
   (locate-dominating-file default-directory ".git")
   default-directory))

(defun vitte--compile (subcommand &optional target)

  (let* ((file (or target (vitte--buffer-file)))
         (cmd
          (format "%s %s %s"
                  (shell-quote-argument vitte-binary)
                  subcommand
                  (shell-quote-argument file))))

    (compile cmd)))

(defun vitte-check-buffer ()
  "Run vitte check."
  (interactive)
  (save-buffer)
  (vitte--compile "check"))

(defun vitte-build-buffer ()
  "Run vitte build."
  (interactive)
  (save-buffer)
  (vitte--compile "build"))

(defun vitte-run-buffer ()
  "Run vitte run."
  (interactive)
  (save-buffer)
  (vitte--compile "run"))

(defun vitte-parse-buffer ()
  "Run vitte parse."
  (interactive)
  (save-buffer)
  (vitte--compile "parse"))

(defun vitte-format-buffer ()
  "Format current buffer."
  (interactive)

  (when (buffer-file-name)

    (call-process
     vitte-binary
     nil
     "*Vitte Format*"
     nil
     "fmt"
     (buffer-file-name))

    (revert-buffer t t t)

    (message "Formatted %s"
             (buffer-name))))

(defun vitte-format-buffer-maybe ()
  (when vitte-format-on-save
    (vitte-format-buffer)))

;;;; ------------------------------------------------------------------
;;;; Navigation
;;;; ------------------------------------------------------------------

(defun vitte-beginning-of-defun ()
  (re-search-backward
   "^[[:space:]]*\\(proc\\|flow\\|form\\|pick\\|trait\\|class\\|entry\\|type\\)\\_>"
   nil
   t))

(defun vitte-end-of-defun ()
  (re-search-forward
   "^[[:space:]]*\\(.end\\|}\\)"
   nil
   t))

;;;; ------------------------------------------------------------------
;;;; Electric support
;;;; ------------------------------------------------------------------

(defun vitte-electric-pair-inhibit (c)
  (if (char-equal c ?<)
      t
    nil))

;;;; ------------------------------------------------------------------
;;;; Keymap
;;;; ------------------------------------------------------------------

(defvar vitte-mode-map
  (let ((map (make-sparse-keymap)))

    ;; build
    (define-key map (kbd "C-c C-c")
                #'vitte-check-buffer)

    (define-key map (kbd "C-c C-b")
                #'vitte-build-buffer)

    (define-key map (kbd "C-c C-r")
                #'vitte-run-buffer)

    (define-key map (kbd "C-c C-p")
                #'vitte-parse-buffer)

    (define-key map (kbd "C-c C-f")
                #'vitte-format-buffer)

    ;; navigation
    (define-key map (kbd "M-a")
                #'vitte-beginning-of-defun)

    (define-key map (kbd "M-e")
                #'vitte-end-of-defun)

    map)
  "Keymap for Vitte.")

;;;; ------------------------------------------------------------------
;;;; Major mode
;;;; ------------------------------------------------------------------

;;;###autoload
(define-derived-mode vitte-mode prog-mode "Vitte"
  "Major mode for editing Vitte source files."

  :syntax-table vitte-mode-syntax-table

  ;; syntax highlighting
  (setq-local font-lock-defaults
              '(vitte-font-lock-keywords))

  ;; comments
  (setq-local comment-start "# ")
  (setq-local comment-end "")
  (setq-local comment-start-skip
              "\\(#+\\|//+\\)\\s-*")

  ;; indentation
  (setq-local indent-tabs-mode nil)
  (setq-local tab-width vitte-indent-offset)

  ;; electric
  (setq-local electric-pair-inhibit-predicate
              #'vitte-electric-pair-inhibit)

  ;; imenu
  (setq-local imenu-generic-expression
              vitte-imenu-generic-expression)

  ;; completion
  (add-hook
   'completion-at-point-functions
   #'vitte-completion-at-point
   nil
   t)

  ;; compilation
  (setq-local compile-command
              (format "%s check %s"
                      vitte-binary
                      (or
                       (buffer-file-name)
                       "")))

  ;; formatting
  (add-hook
   'before-save-hook
   #'vitte-format-buffer-maybe
   nil
   t)

  ;; indentation engine
  (when (require 'vitte-indent nil t)
    (setq-local indent-line-function
                #'vitte-indent-line))

  ;; outline support
  (setq-local outline-regexp
              "^[[:space:]]*\\(proc\\|flow\\|form\\|pick\\|trait\\|class\\|entry\\|type\\)\\_>")

  ;; beginning/end defun
  (setq-local beginning-of-defun-function
              #'vitte-beginning-of-defun)

  (setq-local end-of-defun-function
              #'vitte-end-of-defun)

  ;; prettify
  (setq-local prettify-symbols-alist
              '(
                ("proc" . ?λ)
                ("give" . ?→)
                ("return" . ?⟼)
                ("and" . ?∧)
                ("or" . ?∨)
                ("not" . ?¬)
                ("!=" . ?≠)
                ("<=" . ?≤)
                (">=" . ?≥)
                ("->" . ?→)
                ("=>" . ?⇒)
                )))

;;;; ------------------------------------------------------------------
;;;; File associations
;;;; ------------------------------------------------------------------

;;;###autoload
(add-to-list
 'auto-mode-alist
 '("\\.vit\\'" . vitte-mode))

;;;###autoload
(add-to-list
 'auto-mode-alist
 '("\\.vitte\\'" . vitte-mode))

;;;###autoload
(add-to-list
 'auto-mode-alist
 '("\\.vitl\\'" . vitte-mode))

;;;; ------------------------------------------------------------------
;;;; Provide
;;;; ------------------------------------------------------------------

(provide 'vitte-mode)

;;; vitte-mode.el ends here
