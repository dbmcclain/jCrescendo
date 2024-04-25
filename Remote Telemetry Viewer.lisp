
(defpackage :hc-gui
  (:use :common-lisp :com.ral.actors)
  (:export
   ))

(ql:quickload :earspring)

(in-package :hc-gui)

;; -----------------------------------------
;; View telemetry from Crescendo running on some remote host

(defvar *host*
  "sextans.local"
  ;; "rincon.local"
  ;; "fornax.local"
  )
(defvar *live-telemetry-port*  65201)

;; --------------------------------------------------------
;; Live Telemetry...

(capi:define-interface dual-grafs ()
    ()
    (:panes
     (graf-spec plt:articulated-plotter-pane
             :accessor graf-spec
             :visible-min-width  400
             :visible-min-height 300)
     (graf-hcl plt:articulated-plotter-pane
             :accessor graf-hcl
             :visible-min-width  400
             :visible-min-height 300))
    (:layouts
     (main-layout
      capi:row-layout
      '(graf-spec graf-hcl)))
    
    (:default-initargs
     :layout      'main-layout
     :title       "Live Telemetry Data"
     ))

(defvar *bark-freqs*     (map 'vector (um:curry '* 2.5) (vm:framp 12)))
(defvar *bark-aud-freqs* (map 'vector 'earspring::cbr-corr '(0.25 0.5 0.75 1 1.5 2 3 4 6 8 16)))

(defun draw-bark-freqs (pane yorg)
  (loop for freq across *bark-aud-freqs*
        for lbl in '("250" "500" "750" "1k" "1.5k" "2k" "3k" "4k" "6k" "8k")
        do
        (plt:plot pane `(,freq ,freq) '(-150 150)
                  :thick  1
                  :line-dashing '(4 4)
                  :color  :blue
                  :alpha  0.5)
        (plt:draw-text pane lbl
                       `(:data ,freq ,yorg)
                       :align :ctr
                       )))

(defun dual-graph-beh (&optional graf-spec graf-hcl)
  (lambda (cust specl specr gainl gainr)
    (flet ((spec (cust)
             (with-intercepted-errors (cust)
               (plt:with-delayed-update (graf-spec :notifying (once cust))
                 (plt:spline graf-spec *bark-freqs* specl
                             :clear  t
                             :title  "Live Bark Spectrum"
                             :xtitle "Frequency [zBark]"
                             :ytitle "Amplitude [Phon]"
                             :xrange '(0 25)
                             :yrange '(20 90)
                             :color  :blue
                             :thick  2
                             ;; :symbol :circle
                             :legend "Left"
                             )
                 (plt:spline graf-spec *bark-freqs* specr
                             :color  :red
                             :thick  2
                             ;; :symbol :circle
                             :legend "Right"
                             )
                 (draw-bark-freqs graf-spec 22.5)
                 )))
           (hcl (cust)
             (with-intercepted-errors (cust)
               (plt:with-delayed-update (graf-hcl :notifying (once cust))
                 (plt:spline graf-hcl *bark-freqs* gainl
                             :clear  t
                             :title  "Live Correction Gains"
                             :xtitle "Frequency [zBark]"
                             :ytitle "Gain [dB]"
                             :xrange '(0 25)
                             :yrange '(-20 50)
                             ;; :symbol :circle
                             :color  :blue
                             :thick  2
                             :legend "Left"
                             )
                 (plt:spline graf-hcl *bark-freqs* gainr
                             :color  :red
                             ;; :symbol :circle
                             :thick  2
                             :legend "Right"
                             )
                 (draw-bark-freqs graf-hcl -17.3)
                 ))))
      (cond ((and graf-spec
                  (plt::plotter-valid graf-spec)) ;; still displayed?
             (send (fork (create #'spec)
                         (create #'hcl))
                   cust))
            (t
             (let* ((grafs (capi:display
                            (make-instance 'dual-grafs
                                           :title (format nil "Live Telemetry Data: ~A" *host*)
                                           )))
                    (gspec (graf-spec grafs))
                    (ghcl  (graf-hcl grafs)))
               (become (dual-graph-beh gspec ghcl))
               (repeat-send self)
               ))
            ))))

(deflex* dual-graphs
  (serializer (create (dual-graph-beh))
              :timeout 2))

;; ----------------------------------------------------------

(defvar *done*  nil)

(fli:define-foreign-type chimera (nf)
  `(:union
    (ubv  (:c-array :uint8 (* ,nf 4)))
    (sfv  (:c-array :float ,nf))))

(defun ovly (arr nel grp)
  (make-array nel
              :element-type (array-element-type arr)
              :displaced-to arr
              :displaced-index-offset (* grp nel)))

(defun live-view ()
  (let* ((ubvec  (make-array #.(* 4 12 4)
                             :element-type '(unsigned-byte 8)))
         (fpvec  (make-array #.(* 4 12)
                             :element-type 'single-float))
         (specl  (ovly fpvec 12 0))
         (gainl  (ovly fpvec 12 1))
         (specr  (ovly fpvec 12 2))
         (gainr  (ovly fpvec 12 3))
         (ctr    0)
         (start  (get-universal-time)))
    (declare (dynamic-extent ubvec fpvec))
    (unwind-protect
        (block #1=processing
          (fli:with-dynamic-foreign-objects ((p (chimera #.(* 4 12))))
            (let ((ubv  (fli:foreign-slot-pointer p 'ubv))
                  (sfv  (fli:foreign-slot-pointer p 'sfv)))
              (with-open-stream (sock (comm:open-tcp-stream
                                       *host*
                                       *live-telemetry-port*
                                       :element-type '(unsigned-byte 8)
                                       :ipv6         nil
                                       :read-timeout 2
                                       :errorp       t
                                       ))
                (loop
                   (write-byte 0 sock)
                   (clear-input sock)
                   (force-output sock)
                   ;; grab raw data and partition into float vectors
                   (unless (= #.(* 4 12 4) (read-sequence ubvec sock))
                     (print "Connection timeout.")
                     (return-from #1#))
                   (fli:replace-foreign-array ubv ubvec)
                   (fli:replace-foreign-array fpvec sfv)
                 
                   (ask dual-graphs specl specr gainl gainr)
                   
                   (when *done*
                     (return-from #1#))
                   (when (and (< ctr 100)
                              (= (incf ctr) 100))
                     (format t "~%Rate: ~,1F"
                             (/ 1f2 (max 0.01f0
                                         (- (get-universal-time) start)))))
                   )))))
      (print "Live view exiting.")
      (setf *done* nil)
      )))

(defun show-live ()
  (mp:process-run-function 'live () 'live-view))

(defun stop-live ()
  (setf *done* t))

#|
(show-live)
(stop-live)
|#

