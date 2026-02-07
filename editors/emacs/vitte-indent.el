;;; vitte-indent.el --- Indentation for Vitte -*- lexical-binding: t; -*-

(defun vitte--line-starts-with (re)
  (save-excursion
    (back-to-indentation)
    (looking-at re)))

(defun vitte--prev-line-ends-with (re)
  (save-excursion
    (forward-line -1)
    (end-of-line)
    (skip-chars-backward " \t")
    (looking-back re (line-beginning-position))))

(defun vitte-indent-line ()
  "Indent current line for Vitte."
  (interactive)
  (let ((savep (> (current-column) (current-indentation)))
        (indent 0))
    (save-excursion
      (beginning-of-line)
      (cond
       ((bobp) (setq indent 0))
       ((vitte--line-starts-with "}")
        (save-excursion
          (forward-line -1)
          (setq indent (max 0 (- (current-indentation) tab-width)))))
       ((vitte--line-starts-with "\\(case\\|otherwise\\|else\\)\\b")
        (save-excursion
          (forward-line -1)
          (setq indent (max 0 (- (current-indentation) tab-width)))))
       ((vitte--prev-line-ends-with "{")
        (save-excursion
          (forward-line -1)
          (setq indent (+ (current-indentation) tab-width))))
       ((vitte--prev-line-ends-with "\\(case\\|otherwise\\)\\b.*{")
        (save-excursion
          (forward-line -1)
          (setq indent (+ (current-indentation) tab-width))))
       (t
        (save-excursion
          (forward-line -1)
          (setq indent (current-indentation))))))
    (indent-line-to indent)
    (when savep (move-to-column (+ indent (- (current-column) (current-indentation)))))))

(provide 'vitte-indent)
;;; vitte-indent.el ends here
