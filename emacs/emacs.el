(load-file "/usr/share/emacs/site-lisp/cedet-1.1/common/cedet.el")
;;================================常用设置=========================================
;;在启动时禁止显示警告信息
(setq byte-compile-warnings nil)

;;设定行距
(setq default-line-spacing 1)

;;隐藏滚动条
(scroll-bar-mode -1)  

;;自动载入修改的文件
(global-auto-revert-mode)

;;打开图片显示功能
(auto-image-file-mode t)

;;自动换行
(set-default 'truncate-lines t)

;;关闭起动时闪屏
(setq inhibit-startup-message t)

;;显示行号
(setq column-number-mode t)

;; 默认显示 80列就换行
(setq default-fill-column 80)

;;设置缺省模式是text
(setq default-major-mode 'text-mode)

;;指针不要闪，我得眼睛花了
(blink-cursor-mode -1)

;;使用鼠标中键可以粘贴
(setq mouse-yank-at-point t)

;;不产生备份文件
(setq make-backup-files nil)
(setq backup-by-copying t)
;; 语法高亮
(global-font-lock-mode t)
(setq-default cursor-type 'bar)

;; 改变 Emacs 固执的要你回答 yes 的行为。按 y 或空格键表示 yes，n 表示 no。
(fset 'yes-or-no-p 'y-or-n-p)

										;最大化启动
(setq initial-frame-alist '
	  ((top . 0) (left . 0) (width . 142) (height . 49)))

;;支持 emacs 和外部程序的粘贴 
(setq x-select-enable-clipboard t)

;;粘贴于光标处，而不是鼠标指针处
(setq mouse-yank-at-point t)

;;显示行号
(require 'linum)                                                  
(global-linum-mode 1)

;;不要生成临时文件
(setq-default make-backup-files nil)

;;临时文件放到同一个目录下
(setq backup-directory-alist '(("." . "/tmp")))

;;光标靠近鼠标指针时，让鼠标指针自动让开
(mouse-avoidance-mode 'animate)

;;显示行号
;(require 'linum)                                                  
;(global-linum-mode 1)

;; recentf stuff
(require 'recentf)
(recentf-mode 1)
(setq recentf-max-menu-items 15)
(global-set-key "\C-x\ \C-r"
				'recentf-open-files)

;;将错误信息显示在回显区
(condition-case err
    (progn
      (require 'xxx) )
  (error
   (message "Can't load xxx-mode %s" (cdr err))))

;;================================常用设置=========================================


;;================================基本设置=========================================
;;中文配置

;;设置默认读入文件编码
(prefer-coding-system 'utf-8) 
(set-language-environment 'Chinese-GB)
(set-keyboard-coding-system 'utf-8)
(set-clipboard-coding-system 'utf-8)
(set-terminal-coding-system 'utf-8)
(set-buffer-file-coding-system 'utf-8)
(set-default-coding-systems 'utf-8)
(set-selection-coding-system 'utf-8)
(modify-coding-system-alist 'process "*" 'utf-8)
(setq default-process-coding-system '(utf-8 . utf-8))
(setq-default pathname-coding-system 'utf-8)
(set-file-name-coding-system 'utf-8)
(if (fboundp 'tool-bar-mode)
    (tool-bar-mode -1))
(setq inhibit-startup-message t)

;;识别中文标点
(setq sentence-end "\\([。！？]\\|……\\|[.?!][]\"')}]*\\($\\|[ \t]\\)\\)[ \t\n]*")
(setq sentence-end-double-space nil)

;;括号匹配时显示另外一边的括号，而不是烦人的跳到另一个括号
(show-paren-mode t)
(setq show-paren-style 'parentheses)

(set-face-foreground 'show-paren-match "#6C6C6C") ;;定义前景色
(set-face-bold-p 'show-paren-match t)  ;;加粗显示括号匹配
(set-face-background 'show-paren-match "#ADADAD")  ;;定义背景色

;;shell 模式
(autoload 'ansi-color-for-comint-mode-on "ansi-color" nil t)
(add-hook 'shell-mode-hook 'ansi-color-for-comint-mode-on t)

;;标题栏显示文件的完整路径
(setq frame-title-format
      '("%S" (buffer-file-name "%f"
                               (dired-directory dired-directory "%b"))))
;;titlebar display file path
(add-hook 'window-configuration-change-hook
		  (lambda ()
			(setq frame-title-format
				  (concat
				   invocation-name "@" system-name ": "
				   (replace-regexp-in-string
					(concat "/home/" user-login-name) "~"
					(or buffer-file-name "%b"))))))

;;相同buffer名显示格式
(require 'uniquify)
(setq
 uniquify-buffer-name-style 'post-forward)

(setq
 uniquify-separator ":")


;;================================基本设置=========================================

;;================================快捷键设置=======================================
;;关闭当前buffer
(global-set-key [f4] 'kill-buffer)
;;全选
(global-set-key (kbd "s-a") 'mark-whole-buffer)
;;关闭除当前的其余buffer
(global-set-key (kbd "s-k") 'delete-other-windows)
;;保存文件
(global-set-key (kbd "s-d") 'save-buffer)
;;撤销
(global-set-key (kbd "C-z") 'undo)
;;查找文件
(global-set-key (kbd "s-o") 'find-file)
;;复制
(global-set-key (kbd "s-c") 'kill-ring-save)
;;粘贴
(global-set-key (kbd "s-v") 'yank)
;;剪切
(global-set-key (kbd "s-x") 'kill-region)
;;删除当前行
(setq-default kill-whole-line t)
;;设置开始标记
(global-set-key (kbd "s-m") 'set-mark-command)
;;切换缓冲
(global-set-key (kbd "s-b") 'switch-to-buffer)

;;F8显示/隐藏菜单栏 ;; M-x menu-bar-open
(global-set-key (kbd "<f8>") 'menu-bar-mode)

(global-set-key [f3] 'other-window)
;;(global-set-key (kbd "<f12>") 'calendar)

;;WIN+d进入shell
(global-set-key (kbd "s-s") 'shell)

;; Windows-g: 跳到指定行 
(global-set-key (kbd "s-g") 'goto-line)

;;全屏设置
(global-set-key [f11] 'my-fullscreen)
(defun my-fullscreen ()
  (interactive)
  (x-send-client-message
   nil 0 nil "_NET_WM_STATE" 32
   '(2 "_NET_WM_STATE_FULLSCREEN" 0))
  )

;;================================快捷键设置=======================================
;;================================颜色设置=========================================

;;英文字体
(set-default-font "Liberation Mono-13")
;;(set-default-font " -bitstream-Courier 10 Pitch-normal-normal-normal-*-16-*-*-*-m-0-iso10646-1")
;;中文字体
;;(set-fontset-font "fontset-default"
;;				  'han '("Microsoft YaHei" . "unicode-bmp"))
(set-fontset-font "fontset-default"
                'gb18030 '("STXihei" . "unicode-bmp"))

;; 光标颜色设置
(set-cursor-color "white")

;;背景、前景颜色
(set-face-background 'default "gray22")
(set-foreground-color "gray82")

(set-face-foreground 'region "white")
(set-face-background 'region "thistle4")

;;配置一下shell的颜色
(autoload 'ansi-color-for-comint-mode-on "ansi-color" nil t)
(add-hook 'shell-mode-hook 'ansi-color-for-comint-mode-on)
(setq ansi-color-for-comint-mode t)

;;================================颜色设置=========================================

;;================================php=========================================

;;加载php-mode
(add-to-list 'load-path "/usr/share/emacs/site-lisp")
(require 'php-mode)
(require 'multi-web-mode)

(setq mweb-default-major-mode 'html-mode)
(setq mweb-tags '((php-mode "<\\?php\\|<\\? \\|<\\?=" "\\?>")
				  (js-mode "<script +\\(type=\"text/javascript\"\\|language=\"javascript\"\\)[^>]*>" "</script>")
				  (css-mode "<style +type=\"text/css\"[^>]*>" "</style>")
				  (html-mode "<html*" "</html>")
				  ))
(setq mweb-filename-extensions '("php" "htm" "html" "ctp" "phtml" "php4" "php5"))
(multi-web-global-mode 1)

;;=================================php===========================================


;;=================================C语言编程===========================================

(require 'cc-mode)
(c-set-offset 'inline-open 0) 
(c-set-offset 'friend '-) 
(c-set-offset 'substatement-open 0)

(define-key c-mode-base-map [(f5)] 'compile)
(setq compile-command "make")

;; 编译成功后自动关闭窗口
(setq compilation-finish-functions
      (lambda (buf str)
        (when (and (string= (buffer-name buf) "*compilation*")
                   (not (string-match "exited abnormally" str)))
          (run-at-time 0.5 nil 'delete-windows-on buf)
          )))

;; 设置缩进
(setq indent-tabs-mode nil)
(setq default-tab-width 4)
(setq tab-width 4)
(setq tab-stop-list '(4 8 12 16 20 24 28 32 36 40
						44 48 52 56 60 64 68 72 76 80 84 88 92 96))

(defconst my-c-style
  '((c-tab-always-indent        . t)
    (c-comment-only-line-offset . 4)
    (c-hanging-braces-alist     . ((substatement-open after)
                                   (brace-list-open)))
    (c-hanging-colons-alist     . ((member-init-intro before)
                                   (inher-intro)
                                   (case-label after)
                                   (label after)
                                   (access-label after)))
    (c-cleanup-list             . (scope-operator
                                   empty-defun-braces
                                   defun-close-semi))
    (c-offsets-alist            . ((arglist-close . c-lineup-arglist)
                                   (substatement-open . 0)
                                   (case-label        . 4)
                                   (block-open        . 0)
                                   (knr-argdecl-intro . -)))
    (c-echo-syntactic-information-p . t)
    )
  "My C Programming Style")

;; offset customizations not in my-c-style
(setq c-offsets-alist '((member-init-intro . ++)))

;; Customizations for all modes in CC Mode.
(defun my-c-mode-common-hook ()
  ;; add my personal style and set it for the current buffer
  (c-add-style "PERSONAL" my-c-style t)
  ;; other customizations
  (setq tab-width 4
        ;; this will make sure spaces are used instead of tabs
        indent-tabs-mode nil)
  ;; we like auto-newline and hungry-delete
										;  (c-toggle-auto-hungry-state 1)
  ;; key bindings for all supported languages.  We can put these in
  ;; c-mode-base-map because c-mode-map, c++-mode-map, objc-mode-map,
  ;; java-mode-map, idl-mode-map, and pike-mode-map inherit from it.
  (define-key c-mode-base-map "\C-m" 'c-context-line-break)
  )

(add-hook 'c-mode-common-hook 'my-c-mode-common-hook)



;; 回车缩进
(global-set-key "\C-m" 'newline-and-indent)
(global-set-key (kbd "C-<return>") 'newline)

;;google C/C++ coding style
(add-to-list 'load-path "/usr/share/emacs/site-lisp/google-c-style.el")
										;(add-to-list 'load-path "/usr/share/emacs/mylisp/20_indent-vline.el")

;;
(load-file "/usr/share/emacs/site-lisp/aux-line.el")
(require 'aux-line)

;;=================================C语言编程===========================================


;;====================================ido设置===========================================
(require 'ibuffer)
(global-set-key (kbd "C-x C-b") 'ibuffer)

(require 'ido)
(ido-mode t)

(add-hook 'ido-define-mode-map-hook 'ido-my-keys)

(defun ido-my-keys ()
  "Set up the keymap for `ido'."

  ;; common keys
  (define-key ido-mode-map "\C-e" 'ido-edit-input)  
  (define-key ido-mode-map "\t" 'ido-complete) ;; complete partial
  (define-key ido-mode-map "\C-j" 'ido-select-text)
  (define-key ido-mode-map "\C-m" 'ido-exit-minibuffer)
  (define-key ido-mode-map "?" 'ido-completion-help) ;; list completions
  (define-key ido-mode-map [(control ? )] 'ido-restrict-to-matches)
  (define-key ido-mode-map [(control ?@)] 'ido-restrict-to-matches)

  ;; cycle through matches
  (define-key ido-mode-map "\C-r" 'ido-prev-match)
  (define-key ido-mode-map "\C-s" 'ido-next-match)
  (define-key ido-mode-map [right] 'ido-next-match)
  (define-key ido-mode-map [left] 'ido-prev-match)

  ;; toggles
  (define-key ido-mode-map "\C-t" 'ido-toggle-regexp) ;; same as in isearch
  (define-key ido-mode-map "\C-p" 'ido-toggle-prefix)
  (define-key ido-mode-map "\C-c" 'ido-toggle-case)
  (define-key ido-mode-map "\C-a" 'ido-toggle-ignore)

  ;; keys used in file and dir environment
  (when (memq ido-cur-item '(file dir))
    (define-key ido-mode-map "\C-b" 'ido-enter-switch-buffer)
    (define-key ido-mode-map "\C-d" 'ido-enter-dired)
    (define-key ido-mode-map "\C-f" 'ido-fallback-command)

    ;; cycle among directories
    ;; use [left] and [right] for matching files
    (define-key ido-mode-map [down] 'ido-next-match-dir)
    (define-key ido-mode-map [up]   'ido-prev-match-dir)

    ;; backspace functions
    (define-key ido-mode-map [backspace] 'ido-delete-backward-updir)
    (define-key ido-mode-map "\d"        'ido-delete-backward-updir)
    (define-key ido-mode-map [(meta backspace)] 'ido-delete-backward-word-updir)
    (define-key ido-mode-map [(control backspace)] 'ido-up-directory)

    ;; I can't understand this
    (define-key ido-mode-map [(meta ?d)] 'ido-wide-find-dir)
    (define-key ido-mode-map [(meta ?f)] 'ido-wide-find-file)
    (define-key ido-mode-map [(meta ?k)] 'ido-forget-work-directory)
    (define-key ido-mode-map [(meta ?m)] 'ido-make-directory)

    (define-key ido-mode-map [(meta down)] 'ido-next-work-directory)
    (define-key ido-mode-map [(meta up)] 'ido-prev-work-directory)
    (define-key ido-mode-map [(meta left)] 'ido-prev-work-file)
    (define-key ido-mode-map [(meta right)] 'ido-next-work-file)

    ;; search in the directories
    ;; use C-_ to undo this
    (define-key ido-mode-map [(meta ?s)] 'ido-merge-work-directories)
    (define-key ido-mode-map [(control ?\_)] 'ido-undo-merge-work-directory)
    )

  (when (eq ido-cur-item 'file)
    (define-key ido-mode-map "\C-k" 'ido-delete-file-at-head)
    (define-key ido-mode-map "\C-l" 'ido-toggle-literal)
    (define-key ido-mode-map "\C-o" 'ido-copy-current-word)
    (define-key ido-mode-map "\C-v" 'ido-toggle-vc)
    (define-key ido-mode-map "\C-w" 'ido-copy-current-file-name)
    )

  (when (eq ido-cur-item 'buffer)
    (define-key ido-mode-map "\C-b" 'ido-fallback-command)
    (define-key ido-mode-map "\C-f" 'ido-enter-find-file)
    (define-key ido-mode-map "\C-k" 'ido-kill-buffer-at-head)
    ))

(defun steve-ido-choose-from-recentf ()
  "Use ido to select a recently opened file from the `recentf-list'"
  (interactive)
  (if (and ido-use-virtual-buffers (fboundp 'ido-toggle-virtual-buffers))
      (ido-switch-buffer)
    (find-file (ido-completing-read "Open file: " recentf-list nil t))))

(global-set-key [(meta f6)] 'steve-ido-choose-from-recentf)


;;====================================ido设置===========================================

;;================================tramp远程文件编辑=========================================
;;这里特别写一条ange-ftp的原因是ange-ftp和tramp会有点冲突，但如果使用require先ange-ftp再tramp的方式就可以避免.
(require 'ange-ftp)
(require 'tramp)


;;记住密码
(setq password-cache-expiry nil)
(setq tramp-default-method "ssh")
(setq tramp-default-user "root" tramp-default-host "192.168.1.106")
(setq tramp-auto-save-directory "/tmp/")
(setq auto-save-file-name-transforms
	  '(("\\`/[^/]*:\\(.+/\\)*\\(.*\\)" "/tmp/\\2")))

;;================================tramp远程文件编辑=========================================

(autoload 'js2-mode "js2" nil t)
(add-to-list 'auto-mode-alist '("\\.js$" . js2-mode))


;;================================ECB设置====================================

;;(global-ede-mode 1)                      ; Enable the Project management system
;;(semantic-load-enable-code-helpers)      ; Enable prototype help and smart completion
;;(global-srecode-minor-mode 1)            ; Enable template insertion menu


(setq stack-trace-on-error nil)
(add-to-list 'load-path
			 "/usr/share/emacs/site-lisp/ecb-2.40")
(load-file "/usr/share/emacs/site-lisp/ecb-2.40/ecb.el")

(require 'ecb)
(require 'ecb-autoloads)

(defun my-ecb-active-or-deactive () 
  (interactive) 
  (if ecb-minor-mode 
	  (ecb-deactivate) 
	(ecb-activate))) 

(global-set-key (kbd "<f9>") 'my-ecb-active-or-deactive)
;; Disable buckets so that history buffer can display more entries 
(setq ecb-history-make-buckets 'never)   

;;;; 各窗口间切换
(global-set-key [M-left] 'windmove-left)
(global-set-key [M-right] 'windmove-right)
(global-set-key [M-up] 'windmove-up)
(global-set-key [M-down] 'windmove-down)


;;================================ECB设置==============================================

;;===================================CEDET=============================================

(require 'cedet) 
(require 'semantic) 
(require 'semantic-ia) 
(require 'semanticdb) 
(global-ede-mode t) 
(require 'semantic-gcc) 
(global-srecode-minor-mode 1) 
(semantic-load-enable-minimum-features) 
										;(semantic-load-enable-code-helpers) 
										;(semantic-load-enable-guady-code-helpers) 
										;(semantic-load-enable-excessive-code-helpers) 
										;(semantic-load-enable-semantic-debugging-helpers) 
										;(global-semantic-stickyfunc-mode -1);关掉，免得替代tabbar 
(which-function-mode nil)

(semantic-load-enable-code-helpers)
(global-set-key [(s tab)] 'semantic-ia-complete-symbol-menu)

;; 配置Semantic的检索范围:
(setq semanticdb-project-roots
      (list
       (expand-file-name "/home/yang/Code")))

;; 设置semantic cache临时文件的路径，避免到处都是临时文件
(setq semanticdb-default-save-directory "~/.emacs.d/")

;; C/C++语言启动时自动加载semantic对/usr/include的索引数据库
;; (setq semanticdb-search-system-databases t)
;; (add-hook 'c-mode-common-hook
;;           (lambda ()
;;             (setq semanticdb-project-system-databases
;;                   (list (semanticdb-create-database
;;     semanticdb-new-database-class
;;     "/usr/include")))))


;;代码折叠
(global-semantic-tag-folding-mode 1)
(global-set-key (kbd "s-h") 'semantic-tag-folding-fold-block) 
(global-set-key (kbd "s-e") 'semantic-tag-folding-show-block)
;;F12跳转 
(global-set-key [f12] 'semantic-ia-fast-jump)
;;Windows F12跳转回来 
(global-set-key [s-f12] 
                (lambda () 
                  (interactive) 
                  (if (ring-empty-p (oref semantic-mru-bookmark-ring ring)) 
                      (error "Semantic Bookmark ring is currently empty")) 
                  (let* ((ring (oref semantic-mru-bookmark-ring ring)) 
                         (alist (semantic-mrub-ring-to-assoc-list ring)) 
                         (first (cdr (car alist)))) 
                    (if (semantic-equivalent-tag-p (oref first tag) 
                                                   (semantic-current-tag)) 
                        (setq first (cdr (car (cdr alist))))) 
                    (semantic-mrub-switch-tags first))))


;;===================================CEDET=============================================


;;设置一下备份时的版本控制，这样更加安全。
(setq version-control t)
(setq kept-new-versions 3)
(setq delete-old-versions t)
(setq kept-old-versions 2)
(setq dired-kept-versions 1)

;;设置备份时的版本控制，这样更加安全。
(mapcar
 (function (lambda (setting)
			 (setq auto-mode-alist
				   (cons setting auto-mode-alist))))
 '(("\\.xml$".  sgml-mode)
   ("\\\.bash" . sh-mode)
   ("\\.rdf$".  sgml-mode)
   ("\\.session" . emacs-lisp-mode)
   ("\\.l$" . c-mode)
   ("\\.css$" . css-mode)
   ("\\.cfm$" . html-mode)
   ("gnus" . emacs-lisp-mode)
   ("\\.idl$" . idl-mode)))
(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(ecb-options-version "2.40"))
(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )

