#!@GNOMEG@ -s
-*- scheme -*-
!#

;; TO DO:
;; * Undo (needs text widget to work)
;; * Actual editing (likewise)
;; * Selections (likewise)
;; * Save edited file
;; * Make initial size bigger
;; * Save state correctly
;; * Allow user to choose font
;; * Set bg on text widget?
;; * A toolbar and a status area at the bottom
;; * Search/replace (or use ILU?)
;; * Whatever FIXME comments there are
;; * Page setup and Print (should write gtk widgets for these)


(define-module (gnome notepad)
  :use-module (toolkits gtk))

;; FIXME: set up the textdomain here.
;; (bindtextdomain ...)
;; (textdomain "nsearch")

;;;
;;; Variables
;;;

;; True if text needs to be saved.
(define dirty? #f)

;; The name of the associated file, or #f.
(define file-name #f)

;; The Save menu item.
(define save-menu-item #f)

;; The main window.
(define main-window #f)

;; The text widget.
(define text-widget #f)

;; Session id.
(define session-id #f)

;; Client object
(define client (gnome-client-new-default))
(gtk-signal-connect client "save_yourself" 
		    (lambda (phase 
			     save-style shutdown? interact-style fast?)
		      (notepad-save-for-session client phase save-style 
						shutdown? interact-style fast?)
		      ))

;;;
;;; Generic code.
;;;

(define (wait-for waiter)
  (or (waiter)
      (begin
	;; FIXME - return value.
	(gtk-main-iteration)
	(wait-for waiter))))

(define get-file-name
  (let* ((select #f)
	 (done #f)
	 (filename #f)
	 (destroyer (lambda ()
		      (set! select #f)
		      (set! done #t)))
	 (closer (lambda ()
		   (gtk-widget-hide select)
		   (gtk-grab-remove select)
		   (set! done #t)))
	 (ok (lambda ()
	       (gtk-widget-hide select)
	       (gtk-grab-remove select)
	       (set! filename (gtk-file-selection-get-filename select))
	       (set! done #t))))
    (lambda (title)
      (if select
	  (gtk-window-set-title select title)
	  (begin
	    (set! select (gtk-file-selection-new title))
	    (gtk-signal-connect select "destroy" destroyer)
	    (gtk-signal-connect select "delete_event" closer)
	    (gtk-signal-connect (gtk-file-selection-ok-button select)
				"clicked" ok)
	    (gtk-signal-connect (gtk-file-selection-cancel-button select)
				"clicked" closer)))
      (gtk-widget-show select)
      (gtk-grab-add select)
      (set! done #f)
      (wait-for (lambda () done))
      filename)))

;; FIXME: allow for some icon to be displayed.
(define yes-no-cancel-message-box
  (let* ((dialog #f)
	 (label-widget #f)
	 (result #f)
	 (done #f)
	 (destroyer (lambda ()
		      (set! dialog #f)
		      (set! done #t)))
	 (closer (lambda (val)
		   (set! result val)
		   (gtk-widget-hide dialog)
		   (gtk-grab-remove dialog)
		   (set! done #t))))
    (lambda (title text)
      ;; Default is cancel.
      (set! result 'cancel)
      (if (not dialog)
	  (begin
	    (set! dialog (gtk-dialog-new))
	    (gtk-signal-connect dialog "destroy" destroyer)
	    (gtk-signal-connect dialog "delete_event" closer)

	    (set! label-widget (gtk-label-new text))
	    (gtk-box-pack-start (gtk-dialog-vbox dialog) label-widget
				#f #t 0)
	    (gtk-widget-show label-widget)

	    (let ((yes (gnome-stock-button 'yes))
		  (no (gnome-stock-button 'no))
		  (cancel (gnome-stock-button 'cancel)))
	      (gtk-signal-connect yes "clicked"
				  (lambda ()
				    (closer 'yes)))
	      (gtk-signal-connect no "clicked"
				  (lambda ()
				    (closer 'no)))
	      (gtk-signal-connect cancel "clicked"
				  (lambda ()
				    (closer 'cancel)))

	      (gtk-box-pack-start (gtk-dialog-action-area dialog) yes)
	      (gtk-box-pack-start (gtk-dialog-action-area dialog) no)
	      (gtk-box-pack-start (gtk-dialog-action-area dialog) cancel)
	      (gtk-widget-show-multi yes no cancel))))
      (gtk-window-set-title dialog title)
      (gtk-label-set label-widget text)
      (gtk-widget-show dialog)
      (gtk-grab-add dialog)
      (set! done #f)
      (wait-for (lambda () done))
      result)))

;; Fill a text widget with file contents.
(define (fill-text-widget text file)
  ;; FIXME: error handling.
  (let ((port (open-input-file file)))
    (letrec ((insert-file (lambda ()
                            (let ((line (read-line port 'split)))
                              (if (not (eof-object? (cdr line)))
                                  (let ((str (string-append (car line) "\n")))
                                    (gtk-text-insert text #f #f #f str -1)
                                    (insert-file))
                                  (close-input-port port))))))
      (gtk-text-freeze text)
      (gtk-widget-realize text)
      (gtk-text-set-point text 0)
      (gtk-text-forward-delete text (gtk-text-get-length text))
      (insert-file)
      (gtk-text-thaw text)
      (gnome-history-recently-used file "text/plain" "notepad" "FIXME"))))

;;;
;;; Fluff.
;;;

;; FIXME: if about box already up, don't show it again.
(define (about-box)
  (gtk-widget-show (gnome-about (gettext "Gnome Notepad")
				"0.0"	; FIXME
				(gettext "Copyright (C) 1998 Free Software Foundation")
				(gettext "Gnome Notepad is a program for simple text editing")
				"" ; No pixmap for now.
				"Tom Tromey")))

;;;
;;; Notepad code.
;;;

(define (set-dirty)
  (set! dirty? #t)
  (if file-name
      (gtk-widget-set-sensitive save-menu-item #t)))

(define (clear-dirty)
  (set! dirty? #f)
  (gtk-widget-set-sensitive save-menu-item #f))

(define (set-file-name name)
  (set! file-name name)
  (let ((trans (gettext "Gnome Notepad")))
    (gtk-window-set-title main-window
			  (if name
			      (string-append name " - " trans)
			      trans))))

;; If dirty, query the user if he wants to save.  Returns 'ok (meaning
;; user wants to continue) or 'cancel (user hit Cancel button).
(define (query-for-save)
  (if dirty?
      (let ((result (yes-no-cancel-message-box (gettext "File Modified")
					       "FIXME: Blah Blah Blah")))
	(cond
	 ((eq? result 'yes)
	  (FIXME save it))
	 ((eq? result 'no)
	  'ok)
	 (t
	  'cancel)))
      'ok))

(define (confirm-exit)
  (or (eq? (query-for-save) 'cancel)
      (gtk-exit)))

(define (notepad-close)
  (or (eq? (query-for-save) 'cancel)
      (begin
	(gtk-text-freeze text-widget)
	(gtk-widget-realize text-widget)
	(gtk-text-set-point text-widget 0)
	(gtk-text-forward-delete text-widget
				  (gtk-text-get-length text-widget))
	(clear-dirty)
	(set-file-name #f))))

(define (notepad-open)
  (or (eq? (query-for-save) 'cancel)
      (let ((file (get-file-name (gettext "Open File"))))
	(and file
	     (begin
	       (set-file-name file)
	       (clear-dirty)
	       (fill-text-widget text-widget file))))))

(define (notepad-save)
  (if file-name
      (begin
	(FIXME actually save contents)
	(clear-dirty))))

(define (notepad-save-as)
  (let ((file (get-file-name (gettext "Save File"))))
    (if file
	(begin
	  (set! file-name file)
	  (notepad-save)))))

(define (FIXME . rest)
  #f)

(define (add-menu-item menu label command)
  (let ((item (gnome-stock-menu-item 'blank label)))
    (gtk-signal-connect item "activate" command)
    (gtk-menu-append menu item)
    (gtk-widget-show item)
    item))

(define (add-stock-menu-item menu type label command)
  (let ((item (gnome-stock-menu-item type label)))
    (gtk-signal-connect item "activate" command)
    (gtk-menu-append menu item)
    (gtk-widget-show item)
    item))

(define (file-menu)
  (let ((menu (gtk-menu-new)))
    (add-stock-menu-item menu 'open (gettext "Open...") notepad-open)
    ;; FIXME: should be stock.
    (add-menu-item menu (gettext "Close") notepad-close)
    (set! save-menu-item (add-stock-menu-item menu 'save (gettext "Save")
					      notepad-save))
    (add-menu-item menu (gettext "Save As...") notepad-save-as)
    ;; This is just for debugging; we'll remove it later.
    (add-menu-item menu "Save session (debugging only)"
		   (lambda ()
		     (gnome-client-request-save client 'both #f 'any #f #t)))
    (add-stock-menu-item menu 'exit (gettext "Exit") confirm-exit)
    menu))

(define (edit-menu)
  (let ((menu (gtk-menu-new)))
    ;; FIXME: should be stock.
    (add-menu-item menu (gettext "Undo") FIXME)
    (add-stock-menu-item menu 'copy (gettext "Copy") FIXME)
    (add-stock-menu-item menu 'cut (gettext "Cut") FIXME)
    (add-stock-menu-item menu 'paste (gettext "Paste") FIXME)
    menu))

(define (help-menu)
  (let ((menu (gtk-menu-new)))
    (add-stock-menu-item menu 'about (gettext "About Notepad") about-box)
    menu))

(define (add-menu menu-bar menu label)
  (let ((item (gtk-menu-item-new-with-label label)))
    (gtk-menu-item-set-submenu item menu)
    (gtk-menu-bar-append menu-bar item)
    (gtk-widget-show item)))

(define (menubar)
  (let ((mbar (gtk-menu-bar-new)))
    (add-menu mbar (file-menu) (gettext "File"))
    (add-menu mbar (edit-menu) (gettext "Edit"))
    ;; FIXME: right justify.
    (add-menu mbar (help-menu) (gettext "Help"))
    (gtk-widget-show mbar)
    mbar))

(define (scrolled-text)
  (let* ((hadj (gtk-adjustment-new 0 0 0 0 0 0))
	 (vadj (gtk-adjustment-new 0 0 0 0 0 0))
	 (table (gtk-table-new 2 2 #f))
	 (hscroll (gtk-hscrollbar-new hadj))
	 (vscroll (gtk-vscrollbar-new vadj))
	 (text (gtk-text-new hadj vadj)))
    (set! text-widget text)
    (gtk-table-attach table hscroll 0 1 1 2 '(fill expand) '())
    (gtk-table-attach table vscroll 1 2 0 1 '() '(fill expand))
    (gtk-table-attach-defaults table text 0 1 0 1)
    (gtk-widget-show table)
    (gtk-widget-show hscroll)
    (gtk-widget-show vscroll)
    (gtk-widget-show text)
    (gtk-text-thaw text)
    ;; FIXME: connect to all signals required to handle Undo.
    (gtk-signal-connect text "changed" set-dirty)
    (gtk-text-set-editable text #t)
    table))

(define (notepad)
  (let* ((window (gtk-window-new 'toplevel))
	 (vbox (gtk-vbox-new #f 0)))
    ;; Main window stuff.
    (gtk-signal-connect window "delete_event" (lambda (ev) #t))
    (gtk-signal-connect window "destroy" confirm-exit)

    ;; Make the menu bar.
    (gtk-box-pack-start vbox (menubar) #f #t 0)

    ;; FIXME: a toolbar?

    ;; Make the text edit area.
    (gtk-container-add vbox (scrolled-text))

    ;; FIXME: a status area.

    (gtk-widget-show vbox)
    (gtk-container-add window vbox)

    window))

(define (notepad-save-for-session client phase 
				  save-style shutdown? interact-style fast?)
  ;; FIXME: things to save:
  ;; * window geometry - no way to get this with current guile/gtk.
  ;; * cursor position
  ;; * font
  ;; * scrollbar position
  ;; * dirty flag
  ;; * undo history, when we have it.
  (let ((program (car (program-arguments)))
	(command '()))
    (if file-name
	(set! command (cons (string-append "--file=" file-name) command)))

    ;; This command restarts the program but doesn't supply the
    ;; session id.
    (gnome-client-set-clone-command client (cons program command))

    ;; Restart the command with the session id.
    (gnome-client-set-restart-command client (cons program command)) #t)
  #t)


;; Parse command line options.
(define (notepad-parse-options option arg)
  (if (equal? option "file")
      (begin
	(set! file-name arg)
	#t)
      #f))

(gnome-client-set-current-directory client (getcwd))
(gnome-init-hack "notepad" notepad-parse-options
		 (list (list "file"
			     (gettext "File to open")
			     (gettext "FILE"))))

(set! main-window (notepad))

(set-file-name file-name)
(clear-dirty)
;; If file was specified on command line, load it now.
(if file-name
    (fill-text-widget text-widget file-name))

(gtk-widget-show main-window)
(gtk-main)
