#!@GNOMEG@ -s
-*- scheme -*-
!#

(define-module (gnome search)
  :use-module (toolkits gtk))


;; FIXME: set up the textdomain here.
;; (bindtextdomain ...)
;; (textdomain "nsearch")

(define modified #f)

(define (confirm-exit)
  (if modified
      ()
      (gtk-exit)))

(define search-type-list
  (list "Files named" "Files containing" "Files newer than" 
	"Files older than" "Files owned by" "Files with group"))

(define search-handlers
  `(("Files named" . ,(lambda (val)
			(if (string=? val "")
			    ""
			    (string-append " -name "
					   (shell-quote val)))))
    ("Files containing" . ,(lambda (val)
			     (if (string=? val "")
				 ""
				 (string-append 
				  " -exec sh -c grep\\ "
				  (shell-quote val)
				  "\\ \\{\\}\\ \\>\\ /dev/null \\;"))))
    ("Files newer than" . ,(lambda (val) ""))
    ("Files older than" . ,(lambda (val) ""))
    ("Files owned by" . ,(lambda (val)
			   (if (string=? val "")
			       ""
			       (string-append " -user "
					      (shell-quote val)))))
    ("Files with group" . ,(lambda (val)
			     (if (string=? val "")
				 ""
				 (string-append " -group "
						(shell-quote val)))))))

(define (make-top-search-window)
  (letrec ((window (gtk-window-new 'toplevel))
	   (location-panel (make-location-panel 
			    (lambda (location-specs)
			      (command-buttons-set-location command-buttons
							    location-specs))))
	   (panel-separator (gtk-hseparator-new))
	   (search-panel (make-search-panel 
			  (lambda (search-specs)
			    (command-buttons-set-search command-buttons
							search-specs))))
	   (command-buttons (make-command-buttons
			     (lambda () 
			       (search-panel-add-line search-panel)))))
    (let ((panel-vbox (gnome-make-filled-vbox 
		       #f 6 
		       (gnome-boxed-widget location-panel)
		       (gnome-boxed-widget panel-separator)
		       (gnome-boxed-widget search-panel)
		       (gnome-boxed-widget command-buttons))))

      (gtk-container-border-width window 10)
      (gtk-window-set-title window (gettext "GNOME Search"))
      (gtk-window-set-policy window #f #f #t)
      (gtk-container-add window panel-vbox)
      
      (gtk-signal-connect window "delete_event" (lambda (ev) #t))
      (gtk-signal-connect window "destroy" confirm-exit)
      (gtk-widget-show panel-vbox)
      window)))




(define (make-location-panel notifier)
  (let* ((location-spec '((directory . "") 
			  (subdir . #t)
			  (follow . #f)
			  (xdev . #t)
			  (locate . #f)
			  (max-depth . "Unlimited")))
	 (directory-label (gtk-label-new "Search directory:"))
	 (directory-entry (gtk-entry-new))
	 (directory-line  (gnome-make-filled-hbox 
			   #f 6
			   (gnome-boxed-widget directory-label)
			   (gnome-boxed-widget #t #t 0 directory-entry)))
	 (subdir-check-button 
	  (gtk-check-button-new-with-label "Search subdirectories"))

	 (advanced-options-expander (gnome-make-expander-button 
				     "Advanced options" #f))

	 (follow-check-button
	  (gtk-check-button-new-with-label "Follow symbolic links"))
	 (xdev-check-button
	  (gtk-check-button-new-with-label "Follow mount points"))
	 ;; should stat "/var/lib/locatedb" (or whatever) to
	 ;; get the date and time here.
	 (locate-check-button
	  (gtk-check-button-new-with-label 
	   "Use index created [date, time]"))
	 (depth-label (gtk-label-new "Maximum search depth:"))
	 (depth-entry (gtk-entry-new))
	 (depth-line  (gnome-make-filled-hbox 
		 #f 6
		 (gnome-boxed-widget depth-label)
		 (gnome-boxed-widget depth-entry)))
	 (basic-options (gnome-make-filled-hbox
		 #f 6 (gnome-boxed-widget subdir-check-button)
		 'pack-end
		 (gnome-boxed-widget advanced-options-expander)
		 ))
	 (advanced-options (gtk-table-new 2 2 #f))
	 (make-toggler-notifier 
	  (lambda (key)
	    (lambda ()
	      (assq-set! location-spec key
			 (not (cdr (assq key location-spec))))
	      (notifier location-spec))))
	 (location-panel #f))

    ;; Really evil! Don't hard-code pixel widths!
    (gtk-widget-set-usize depth-entry 50 0)

    (gtk-table-set-row-spacings advanced-options 6)
    (gtk-table-set-col-spacings advanced-options 6)
    (gtk-table-attach-defaults advanced-options 
			       follow-check-button 0 1 0 1)
    (gtk-widget-show follow-check-button)
    (gtk-table-attach-defaults advanced-options 
			       xdev-check-button 1 2 0 1)
    (gtk-widget-show xdev-check-button)
    (gtk-table-attach-defaults advanced-options 
			       locate-check-button 0 1 1 2)
    (gtk-widget-show locate-check-button)
    (gtk-table-attach-defaults advanced-options 
			       depth-line 1 2 1 2)
    (gtk-widget-show depth-line)

    (set! location-panel
	  (gnome-make-filled-vbox 
	   #f 6 
	   (gnome-boxed-widget directory-line)
	   (gnome-boxed-widget basic-options)
	   (gnome-boxed-widget advanced-options)))

    (gnome-expander-button-add-widgets advanced-options-expander
				       advanced-options)

    (gtk-toggle-button-set-state subdir-check-button #t)
    (gtk-toggle-button-set-state follow-check-button #f)
    (gtk-toggle-button-set-state xdev-check-button #t)
    (gtk-toggle-button-set-state locate-check-button #f)

    (gtk-signal-connect directory-entry "changed"
			(lambda ()
			  (assq-set! location-spec 'directory
				     (gtk-entry-get-text directory-entry))
			  (notifier location-spec)))

    (gtk-signal-connect subdir-check-button
			"toggled" (make-toggler-notifier 'subdir))
    (gtk-signal-connect follow-check-button
			"toggled" (make-toggler-notifier 'follow))
    (gtk-signal-connect xdev-check-button
			"toggled" (make-toggler-notifier 'xdev))
    (gtk-signal-connect locate-check-button
			"toggled" (make-toggler-notifier 'locate))

    (gtk-signal-connect depth-entry "changed"
			(lambda ()
			  (assq-set! location-spec 'depth
				     (gtk-entry-get-text depth-entry))
			  (notifier location-spec)))

    location-panel))

(define (make-search-panel notifier)
  (let* ((search-specs (list '("Files named" . "")))
	 (updater-proc (lambda (spec index)
			 (list-set! search-specs index spec)
			 (notifier search-specs)))
	 (search-panel-vbox (gnome-make-filled-vbox
			     #f 6
			     (gnome-boxed-widget 
			      (make-search-panel-line 
			      (lambda (search-spec)
				(updater-proc search-spec 0))))))
	 (counter 1))
    (set-object-property! search-panel-vbox 'search-panel-add-line
			  (lambda ()
			    (let* ((lcounter counter)
				   (new-line
				    (make-search-panel-line 
				     (lambda (search-spec)
				       (updater-proc search-spec lcounter)))))
			      (gtk-box-pack-start search-panel-vbox
						   new-line
						   #f #f 0)
			      (gtk-widget-show new-line)
			      (set! search-specs (append search-specs 
							 (list '("Files named" 
								 .
								 ""))))
			      (set! counter (+ 1 counter)))))
    search-panel-vbox
  ))

(define (search-panel-add-line search-panel)
  (cond
   ((object-property search-panel 'search-panel-add-line)
    => (lambda (proc) (proc)))
   (else (error 'wrong-type-arg seacrh-panel))))


(define (make-search-panel-line notifier)
  (let* ((spec (cons "Files named" ""))
	 (search-type (make-search-type-option-menu 
		       (lambda (t) 
			 (set-car! spec t)
			 (notifier spec))))
	 (search-entry (gtk-entry-new))
	 (search-spec-hbox (gnome-make-filled-hbox 
			    #f 6
			    (gnome-boxed-widget search-type)
			    (gnome-boxed-widget #t #t 0 search-entry))))
    (gtk-signal-connect search-entry "changed"
			(lambda ()
			  (set-cdr! spec (gtk-entry-get-text search-entry))
			  (notifier spec)))
    search-spec-hbox))


(define (make-search-type-option-menu select-action-proc)
  (let ((search-type-option-menu (gtk-option-menu-new))
	(search-type-menu (gtk-menu-new)))

    (for-each (lambda (item) 
		(let ((menu-item (gtk-menu-item-new-with-label item)))
		  (gtk-menu-append search-type-menu menu-item)
		  (gtk-widget-show menu-item)
		  (gtk-signal-connect menu-item "activate" 
				      (lambda ()
					(select-action-proc item)
					))))
	      search-type-list)
    
    (gtk-option-menu-set-menu search-type-option-menu search-type-menu)

    search-type-option-menu
    ))




(define (make-command-buttons search-add-notify)
  (let* ((location-spec '())
	 (search-spec '())
	 (command-line-expander 
	  (gnome-make-expander-button "Command line" #f))
	 (add-search (gtk-button-new-with-label 
		      (gettext "Add search condition >>"))) 
	 (search (gtk-button-new-with-label (gettext "Search")))
	 (exit (gtk-button-new-with-label (gettext "Exit")))
	 
	 (cl-separator (gtk-hseparator-new))
	 (command-line (gtk-entry-new))
	 
	 (button-hbox (gnome-make-filled-hbox 
		       #f 6 'pack-end
		       (gnome-boxed-widget exit)
		       (gnome-boxed-widget search)
		       (gnome-boxed-widget add-search)
		       (gnome-boxed-widget command-line-expander)))

	 (search-command "find . - print")

	 (update (lambda ()
		   (set! search-command 
			 (make-search-command location-spec search-spec))
		   (gtk-entry-set-text command-line search-command)))

	 (command-buttons (gnome-make-filled-vbox 
			   #f 6
			   (gnome-boxed-widget button-hbox) 
			   (gnome-boxed-widget cl-separator) 
			   (gnome-boxed-widget command-line))))

    (gtk-entry-set-text command-line search-command)

    (gtk-signal-connect exit "clicked" confirm-exit)
    (gtk-signal-connect add-search "clicked" search-add-notify)
    (gtk-signal-connect search "clicked" (lambda () 
					   (execute-search search-command)))

    (gnome-expander-button-add-widgets command-line-expander
				       cl-separator
				       command-line)
    (set-object-property! command-buttons 'command-buttons-set-location
			 (lambda (l) 
			   (set! location-spec l)
			   (update)))

    (set-object-property! command-buttons 'command-buttons-set-search
			 (lambda (s) 
			   (set! search-spec s)
			   (update)))

    command-buttons))


(define (command-buttons-set-location command-buttons location)
  (cond
   ((object-property command-buttons 'command-buttons-set-location)
    => (lambda (proc) (proc location)))
   (else (error 'wrong-type-arg command-buttons-set-location))))


(define (command-buttons-set-search command-buttons search)
  (cond
   ((object-property command-buttons 'command-buttons-set-search)
    => (lambda (proc) (proc search)))
   (else (error 'wrong-type-arg command-buttons-set-search))))

(define (shell-quote s)
  (string-append "\"" s "\""))

(define (make-location-prefix location)
  (if (not (null? location))
      (string-append "find "
		     (shell-quote (cdr (assoc 'directory location )))
		     (if (cdr (assoc 'follow location))
			 " -follow"
			 "")
		     (if (cdr (assoc 'xdev location))
			 ""
			 " -xdev")
		     (cond
		      ((not (cdr (assoc 'subdir location)))
		       " -maxdepth 1")
		      ((string->number (cdr (assoc 'max-depth location)))
		       (string-append " -maxdepth " 
				      (cdr (assoc 'max-depth location))))
		      (else "")))
      "find ."))
		 
(define (make-find-body search)
  (apply string-append
	 (map (lambda (s)
		((cdr (assoc (car s) search-handlers)) (cdr s)))
	      search)))


(define (make-search-command location search)
  (string-append (make-location-prefix location)
		 (make-find-body search)
		 " -print"))

;; what we really need to do is keep running the event loop in a cancel
;; dialog, killing the search process on cancel, and waitpid-ing on
;; it the find suborocess with the WNOHANG flag. 

(define (execute-search command)
  (if (= 0 (primitive-fork))
      (system command)))



;;; gnome-make-expander-button name state . widgets
;;;   name is the name to use for the label 
;;;     (" >>" or " <<" will be appended depending on the state)
;;;   state is the initial state, #f for collapsed, #t for expanded
;;;   widgets is the list of widgets initially under the expander's
;;;   control.
;;;
;;;   Creates an "expander button", a button that makes some widgets
;;;   show or hide in the current dialog. It is very useful for making
;;;   expandable dialogs that show some basic options by default but
;;;   can be expanded to show more. The object returned can be treated
;;;   as a gtk-button in all respects, but some extra procedures
;;;   (below) work on it.

(define (gnome-make-expander-button name state . widgets)
  (let* ((state #f)
	 (expand-name (string-append name " >>"))
	 (collapse-name (string-append name " <<"))
	 (label (gtk-label-new expand-name))
	 (button (gtk-button-new))
	 (set-state! (lambda (new-state)
		       (cond
			(new-state
			 (gtk-label-set label collapse-name)
			 (map gtk-widget-show widgets))
			(else 
			 (gtk-label-set label expand-name)
			 (map gtk-widget-hide widgets)))
		       (set! state new-state)))
	 (handler (lambda ()
		    (set-state! (not state))))
	 )
    
    (gtk-container-add button label)
    (gtk-widget-show label)
    
    (set-state! state)

    (gtk-signal-connect button "clicked" handler)

    (set-object-property! button 'gnome-expander-button? #t)
    (set-object-property! button 'gnome-expander-button-get-state
			  (lambda () state))
    (set-object-property! button 'gnome-expander-button-set-state
			  set-state!)
    (set-object-property! button 'gnome-expander-button-add-widgets
			  (lambda (ws)
			    (map
			     (lambda (widget)
			       (cond 
				((not (memq widget widgets))
				 (set! widgets (cons widget widgets))
				 (if state
				     (gtk-widget-show widget)
				     (gtk-widget-hide widget)))))
			     ws)))
    (set-object-property! button 'gnome-expander-button-remove-widgets
			  (lambda (ws)
			    (map
			     (lambda (widget)
			       (if (not (memq widget widgets))
				   (set! widgets (delq! widget widgets))))
			     ws)))
    button))

;;; gnome-expander-button? object
;;;   true if object is a gnome-expander-button (gtk-button? should also
;;;   be true of it).

(define (gnome-expander-button? object)
  (object-property object 'gnome-expander-button?))

;;; gnome-expander-button-get-state expander-button
;;;   retrieves the current state of the given expander button

(define (gnome-expander-button-get-state expander-button)
  (cond
   ((object-property expander-button 'gnome-expander-button-get-state)
    => (lambda (proc) (proc)))
   (else (error 'wrong-type-arg expander-button))))

;;; gnome-expander-button-set-state expander-button new-state
;;;   sets the state of the given expander button to new-state

(define (gnome-expander-button-set-state expander-button new-state)
  (cond
   ((object-property expander-button 'gnome-expander-button-set-state)
    => (lambda (proc) (proc new-state)))
   (else (error 'wrong-type-arg expander-button))))

;;; gnome-expander-button-add-widget expander-button widget
;;;   adds wdigets to the control of the expander-button. The
;;;   widgets are hidden or shown to match the current state.

(define (gnome-expander-button-add-widgets expander-button . widgets)
  (cond
   ((object-property expander-button 'gnome-expander-button-add-widgets)
    => (lambda (proc) (proc widgets)))
   (else (error 'wrong-type-arg expander-button))))

;;; gnome-expander-button-remove-widget expander-button widget
;;;   removes wdigets from the control of the expander-button. The
;;;   widgets remains in the hide/show state they were in until they are
;;;   explicitly hidden or shown.

(define (gnome-expander-button-remove-widgets expander-button . widgets)
  (cond
   ((object-property expander-button 'gnome-expander-button-remove-widgets)
    => (lambda (proc) (proc widgets)))
   (else (error 'wrong-type-arg expander-button))))


(define top-search-window (make-top-search-window))
(gtk-widget-show top-search-window)

(gtk-main)
