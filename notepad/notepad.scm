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
;; * Write About box (it would be nice to have a standard function)
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
      (if dialog
	  (begin
	    (gtk-window-set-title dialog title)
	    (gtk-label-set label-widget text))
	  (begin
	    (set! dialog (gtk-dialog-new))
	    (gtk-signal-connect dialog "destroy" destroyer)
	    (gtk-signal-connect dialog "delete_event" closer)

	    (set! label-widget (gtk-label-new text))
	    (gtk-box-pack-start (gtk-dialog-vbox dialog) label-widget
				#f #t 0)
	    (gtk-widget-show label-widget)

	    (let ((yes (gtk-button-new-with-label (gettext "Yes")))
		  (no (gtk-button-new-with-label (gettext "No")))
		  (cancel (gtk-button-new-with-label (gettext "Cancel"))))
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
      (gtk-text-foreward-delete text (gtk-text-get-length text))
      (insert-file)
      (gtk-text-thaw text))))

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
	(gtk-text-foreward-delete text-widget
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
  (let ((item (gtk-menu-item-new-with-label label)))
    (gtk-signal-connect item "activate" command)
    (gtk-menu-append menu item)
    (gtk-widget-show item)
    item))

(define (file-menu)
  (let ((menu (gtk-menu-new)))
    (add-menu-item menu (gettext "Open...") notepad-open)
    (add-menu-item menu (gettext "Close") notepad-close)
    (set! save-menu-item (add-menu-item menu (gettext "Save") notepad-save))
    (add-menu-item menu (gettext "Save As...") notepad-save-as)
    (add-menu-item menu (gettext "Exit") confirm-exit)
    menu))

(define (edit-menu)
  (let ((menu (gtk-menu-new)))
    (add-menu-item menu (gettext "Undo") FIXME)
    (add-menu-item menu (gettext "Copy") FIXME)
    (add-menu-item menu (gettext "Cut") FIXME)
    (add-menu-item menu (gettext "Paste") FIXME)
    menu))

(define (help-menu)
  (let ((menu (gtk-menu-new)))
    (add-menu-item menu (gettext "About Gnome Notepad") FIXME)
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
    ;; FIXME: this signal doesn't exist yet.
    ;; (gtk-signal-connect text "changed" set-dirty)
    ;; (gtk-text-set-editable text #t)
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

(define (notepad-save-for-session save-style shutdown? interact-style fast?)
  ;; FIXME: things to save:
  ;; * the file
  ;; * window geometry
  ;; * cursor position, font, scrollbar position
  ;; * dirty flag
  #t)


;; FIXME: parse command-line arguments.

(gnome-session-init notepad-save-for-session
		    ;; We don't care about exiting.
		    (lambda (shutdown?) #f)
		    )

(gnome-session-set-current-directory (getcwd))
(apply gnome-session-set-restart-command (program-arguments))
(gnome-session-set-clone-command (program-arguments))
(gnome-session-set-program (car (program-arguments)))


(set! main-window (notepad))
(set-file-name #f)
(clear-dirty)
(gtk-widget-show main-window)
(gtk-main)
