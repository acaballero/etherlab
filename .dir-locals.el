((nil . ((eval . (let ((marker-var (intern (concat "my-project-loaded-" 
                                                  (file-name-nondirectory 
                                                   (directory-file-name 
                                                    (locate-dominating-file default-directory ".dir-locals.el")))))))
                   (unless (boundp marker-var)
                     (load (expand-file-name "project-init.el" 
                                            (locate-dominating-file default-directory ".dir-locals.el")))
                     (set marker-var t)))))))
