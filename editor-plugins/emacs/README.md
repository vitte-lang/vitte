# Vitte — Emacs

```elisp
(add-to-list 'load-path "path/to/editor-plugins/emacs/")
(require 'vitte-mode)
(add-to-list 'auto-mode-alist '("\\.vitte\\'" . vitte-mode))
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
;; LSP : (require 'lsp-mode) puis (add-hook 'vitte-mode-hook #'lsp-deferred)
```
