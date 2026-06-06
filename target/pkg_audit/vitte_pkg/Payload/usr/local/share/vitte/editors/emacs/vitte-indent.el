;;; vitte-indent.el --- Advanced indentation engine for Vitte -*- lexical-binding: t; -*-

;; High quality indentation engine for the Vitte language.
;; Supports:
;; - braces
;; - legacy .end blocks
;; - match/select/case
;; - multiline calls
;; - chained expressions
;; - continuation indentation
;; - comments
;; - attributes
;; - modern syntax variants

(require 'cl-lib)

(defgroup vitte-indent nil
  "Indentation engine for Vitte."
  :group 'languages)

(defcustom vitte-indent-offset 4
  "Indent width for Vitte."
  :type 'integer
  :group 'vitte-indent)

(defcustom vitte-indent-continuation-offset 4
  "Continuation indentation width."
  :type 'integer
  :group 'vitte-indent)

(defconst vitte--block-open-keywords
  '("if"
    "elif"
    "else"
    "otherwise"
    "match"
    "select"
    "when"
    "case"
    "loop"
    "while"
    "until"
    "for"
    "each"
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
    "unsafe"
    "asm"
    "try"
    "catch"
    "finally"))

(defconst vitte--dedent-keywords
  '("}"
    ".end"
    "case"
    "otherwise"
    "else"
    "elif"
    "catch"
    "finally"))

(defun vitte--trim-string (s)
  (replace-regexp-in-string
   "[ \t\n\r]+$"
   ""
   (replace-regexp-in-string
    "^[ \t\n\r]+"
    ""
    s)))

(defun vitte--current-line ()
  (buffer-substring-no-properties
   (line-beginning-position)
   (line-end-position)))

(defun vitte--current-line-trimmed ()
  (vitte--trim-string (vitte--current-line)))

(defun vitte--blank-line-p ()
  (string-match-p
   "\\`[ \t]*\\'"
   (vitte--current-line)))

(defun vitte--comment-line-p ()
  (save-excursion
    (back-to-indentation)
    (or
     (looking-at-p "#")
     (looking-at-p "//")
     (looking-at-p "/\\*")
     (looking-at-p "\\*")
     (looking-at-p "<<<"))))

(defun vitte--attribute-line-p ()
  (save-excursion
    (back-to-indentation)
    (looking-at-p "#\$begin:math:display$\"\)\)\)

\(defun vitte\-\-line\-starts\-with \(re\)
  \(save\-excursion
    \(back\-to\-indentation\)
    \(looking\-at re\)\)\)

\(defun vitte\-\-line\-ends\-with \(re\)
  \(save\-excursion
    \(end\-of\-line\)
    \(skip\-chars\-backward \" \\t\"\)
    \(looking\-back re \(line\-beginning\-position\)\)\)\)

\(defun vitte\-\-previous\-code\-line \(\)
  \"Move to previous non\-empty non\-comment line\.\"
  \(let \(\(found nil\)\)
    \(while \(and \(not found\)
                \(\= \(forward\-line \-1\) 0\)\)
      \(unless \(or
               \(vitte\-\-blank\-line\-p\)
               \(vitte\-\-comment\-line\-p\)\)
        \(setq found t\)\)\)
    found\)\)

\(defun vitte\-\-previous\-indentation \(\)
  \(save\-excursion
    \(if \(vitte\-\-previous\-code\-line\)
        \(current\-indentation\)
      0\)\)\)

\(defun vitte\-\-previous\-line\-text \(\)
  \(save\-excursion
    \(if \(vitte\-\-previous\-code\-line\)
        \(vitte\-\-current\-line\-trimmed\)
      \"\"\)\)\)

\(defun vitte\-\-line\-opens\-block\-p \(line\)
  \(or
   \(string\-match\-p \"\{\[ \\t\]\*\$\" line\)
   \(string\-match\-p
    \(concat
     \"\\\\\_\<\"
     \(regexp\-opt vitte\-\-block\-open\-keywords t\)
     \"\\\\\_\>\"\)
    line\)\)\)

\(defun vitte\-\-line\-closes\-block\-p \(\)
  \(save\-excursion
    \(back\-to\-indentation\)
    \(or
     \(looking\-at\-p \"\}\"\)
     \(looking\-at\-p \"\\\\\.end\\\\\_\>\"\)
     \(looking\-at\-p
      \(concat
       \"\\\\\(\"
       \(regexp\-opt
        \'\(\"case\" \"otherwise\" \"else\" \"elif\" \"catch\" \"finally\"\)\)
       \"\\\\\)\\\\\_\>\"\)\)\)\)\)

\(defun vitte\-\-continuation\-line\-p \(\)
  \(save\-excursion
    \(let \(\(prev \(vitte\-\-previous\-line\-text\)\)\)
      \(or
       \(string\-match\-p
        \"\[\,\(\\\\\[\{\:\+\\\\\-\*\/\=\<\>\|\&\!\]\\\\s\-\*\$\"
        prev\)

       \(string\-match\-p
        \"\\\\\_\<\\\\\(and\\\\\|or\\\\\|xor\\\\\|as\\\\\|is\\\\\)\\\\\_\>\\\\s\-\*\$\"
        prev\)

       \(string\-match\-p
        \"\\\\\.\$\"
        prev\)\)\)\)\)

\(defun vitte\-\-inside\-parens\-depth \(\)
  \(car \(syntax\-ppss\)\)\)

\(defun vitte\-\-inside\-string\-or\-comment\-p \(\)
  \(nth 8 \(syntax\-ppss\)\)\)

\(defun vitte\-\-compute\-indent \(\)
  \(save\-excursion
    \(beginning\-of\-line\)

    \(cond

     \;\; file start
     \(\(bobp\)
      0\)

     \;\; keep comments aligned
     \(\(vitte\-\-comment\-line\-p\)
      \(vitte\-\-previous\-indentation\)\)

     \;\; inside multiline string\/comment
     \(\(vitte\-\-inside\-string\-or\-comment\-p\)
      \(current\-indentation\)\)

     \;\; dedent keywords
     \(\(vitte\-\-line\-closes\-block\-p\)
      \(max
       0
       \(\- \(vitte\-\-previous\-indentation\)
          vitte\-indent\-offset\)\)\)

     \;\; attributes
     \(\(vitte\-\-attribute\-line\-p\)
      \(vitte\-\-previous\-indentation\)\)

     \;\; multiline expressions
     \(\(vitte\-\-continuation\-line\-p\)
      \(\+ \(vitte\-\-previous\-indentation\)
         vitte\-indent\-continuation\-offset\)\)

     \;\; parenthesis depth
     \(\(\> \(vitte\-\-inside\-parens\-depth\) 0\)
      \(\+ \(vitte\-\-previous\-indentation\)
         vitte\-indent\-continuation\-offset\)\)

     \;\; previous line opens block
     \(\(vitte\-\-line\-opens\-block\-p
       \(vitte\-\-previous\-line\-text\)\)
      \(\+ \(vitte\-\-previous\-indentation\)
         vitte\-indent\-offset\)\)

     \;\; default
     \(t
      \(vitte\-\-previous\-indentation\)\)\)\)\)

\(defun vitte\-indent\-line \(\)
  \"Indent current line as Vitte code\.\"
  \(interactive\)

  \(let\* \(\(pos \(\- \(point\-max\) \(point\)\)\)
         \(indent \(vitte\-\-compute\-indent\)\)\)

    \(beginning\-of\-line\)
    \(delete\-horizontal\-space\)
    \(indent\-to indent\)

    \;\; preserve cursor
    \(when \(\> \(\- \(point\-max\) pos\) \(point\)\)
      \(goto\-char \(\- \(point\-max\) pos\)\)\)\)\)

\(defun vitte\-indent\-region \(begin end\)
  \"Indent Vitte region\.\"
  \(interactive \"r\"\)
  \(save\-excursion
    \(goto\-char begin\)

    \(while \(\< \(point\) end\)
      \(unless \(vitte\-\-blank\-line\-p\)
        \(vitte\-indent\-line\)\)
      \(forward\-line 1\)\)\)\)

\(defun vitte\-newline\-and\-indent \(\)
  \"Insert newline and indent\.\"
  \(interactive\)
  \(newline\)
  \(vitte\-indent\-line\)\)

\(defun vitte\-electric\-brace \(arg\)
  \"Insert brace and reindent\.\"
  \(interactive \"\*P\"\)

  \(self\-insert\-command \(prefix\-numeric\-value arg\)\)

  \(when \(member last\-command\-event \'\(\?\\\} \?$end:math:display$ ?\)))
    (save-excursion
      (forward-char -1)
      (vitte-indent-line)))

  (vitte-indent-line))

(defun vitte-electric-end ()
  "Insert .end and reindent."
  (interactive)

  (insert ".end")
  (vitte-indent-line)

  (save-excursion
    (forward-line 1)
    (unless (eobp)
      (vitte-indent-line))))

(defvar vitte-indent-mode-map
  (let ((map (make-sparse-keymap)))

    (define-key map (kbd "RET")
                #'vitte-newline-and-indent)

    (define-key map "}"
                #'vitte-electric-brace)

    map)
  "Keymap for vitte indentation helpers.")

;;;###autoload
(define-minor-mode vitte-indent-mode
  "Advanced indentation support for Vitte."
  :lighter " VIndent"
  :keymap vitte-indent-mode-map

  (if vitte-indent-mode
      (setq-local indent-line-function
                  #'vitte-indent-line)
    (kill-local-variable 'indent-line-function)))

(provide 'vitte-indent)

;;; vitte-indent.el ends here
