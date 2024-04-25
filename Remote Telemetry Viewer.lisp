
(defpackage :hc-gui
  (:use :common-lisp :com.ral.actors)
  (:export
   ))

(ql:quickload :earspring)

(in-package :hc-gui)

;; -----------------------------------------
;; Global read-only data vectors

(defvar *host*
  "sextans.local"
  ;; "rincon.local"
  ;; "fornax.local"
  )
(defvar *live-control-port*    65100)
(defvar *live-telemetry-port*  65201)

(defvar *chan*  #( 1     2     3      4     5     6     7     8     9    10  ))
(defvar *frqs*  #( 0.25  0.5   0.75   1.0   1.5   2.0   3.0   4.0   6.0   8.0))
(defvar *elevl* #( 3    12    17     22    33    40    50    57    62    65  ))
(defvar *dhl*   #( 0     0    -3     -7    -3    -3    10    10    10    10  ))
(defvar *elevr* #( 3    12    17     22    33    40    50    57    62    65  ))
(defvar *dhr*   #( 0     0    -3     -7    -3    -3    10    10    10    10  ))

(defun chk-color (color)
  ;; use against a keyword color name, like :GREEN4, to see if it exists,
  ;; else return :MAGENTA so that it stands out, but does not cause a segfault.
  (or (find color (color:get-all-color-names))
      :magenta))

(defun draw-freqs (pane ypos)
  (loop for ix from 1
        for freq in '("250" "500" "750" "1k" "1.5k" "2k" "3k" "4k" "6k" "8k") 
        do
        (plt:plot pane `(,ix ,ix) '(-150 150)
                  :thick 1
                  :line-dashing '(4 4)
                  :color :blue
                  :alpha 0.5)
        (plt:draw-text pane freq
                       `(:data ,ix ,ypos)
                       :align :ctr)))

(defun highlight-measurement (pane arr-l arr-r lr-chan freq-band)
  (when (and freq-band lr-chan)
    (plt:draw-circle pane
                     (aref *chan* freq-band)
                     (aref (if (eql lr-chan :LEFT)
                               arr-l
                             arr-r)
                           freq-band)
                     '(:pix 10)
                     :filled       nil
                     :border-color (chk-color :green4)
                     :border-thick 2)
    ))

(defvar *cur-elevl*  nil)
(defvar *cur-elevr*  nil)

(defun draw-elev-plot (plte elevl elevr lr-chan freq-band)
  (setf *cur-elevl* elevl
        *cur-elevr* elevr)
  ;; left channel
  (plt:with-delayed-update (plte)
    (plt:spline plte *chan* elevl
                :clear  t
                :xtitle "Correction Band"
                :ytitle "Threshold Elevation [dB]"
                :title  "Audiology Threshold Adjustments"
                :thick  2
                :xrange '(0 11)
                :yrange '(0 100)
                :symbol :circle
                :color  :blue
                :alpha  (if (eql lr-chan :RIGHT) 0.3 (if lr-chan 1.0 0.5))
                :legend "Left"
                )
    (draw-freqs plte 3)
    (plt:spline plte *chan* elevr
                :color  :red
                :alpha  (if (eql lr-chan :LEFT) 0.3 (if lr-chan 1.0 0.5))
                :thick  2
                :symbol :circle
                :legend "Right"
                )
    (highlight-measurement plte elevl elevr lr-chan freq-band)
    ))

(defvar *cur-dhl*  nil)
(defvar *cur-dhr*  nil)

(defun draw-dh-plot (plth dhl dhr lr-chan freq-band)    
  (setf *cur-dhl* dhl
        *cur-dhr* dhr)
  ;; left channel
  (plt:with-delayed-update (plth)
    (plt:spline plth *chan* dhl
                :clear  t
                :xtitle "Correction Band"
                :ytitle "+D/-H Level [dB]"
                :title  "Audiology +D/-H Adjustments"
                :thick  2
                :xrange '(0 11)
                :yrange '(-18 18)
                :symbol :circle
                :color  :blue
                :alpha  (if (eql lr-chan :RIGHT) 0.3 (if lr-chan 1.0 0.5))
                :legend "Left"
                )
    (draw-freqs plth -17)
    (plt:spline plth *chan* dhr
                :color  :red
                :alpha  (if (eql lr-chan :LEFT) 0.3 (if lr-chan 1.0 0.5))
                :thick  2
                :symbol :circle
                :legend "Right"
                )
    (highlight-measurement plth dhl dhr lr-chan freq-band)
    ))

(defun lr-band-data-beh (plotter-fn select-msg cresc arr-l arr-r l/r freq-band)
  (alambda
   ((cust :show)
    (funcall plotter-fn arr-l arr-r l/r freq-band)
    (send cust :ok))

   ((cust :select new-l/r new-band)
    (send self cust :show)
    (become (lr-band-data-beh plotter-fn select-msg cresc arr-l arr-r new-l/r new-band))
    (case (and new-band new-l/r)
      (:LEFT
       (send cust select-msg (aref arr-l new-band)))
      (:RIGHT
       (send cust select-msg (aref arr-r new-band)))
      ))

   ((cust :update val) when (and l/r freq-band)
    (send self cust :show)
    (case l/r
      (:LEFT
       (let ((new-arr (copy-seq arr-l)))
         (setf (aref new-arr freq-band) val)
         (send cresc freq-band val)
         (become (lr-band-data-beh plotter-fn select-msg cresc new-arr arr-r l/r freq-band))))
      (:RIGHT
       (let ((new-arr (copy-seq arr-r)))
         (setf (aref new-arr freq-band) val)
         (send cresc (+ 10 freq-band) val)
         (become (lr-band-data-beh plotter-fn select-msg cresc arr-l new-arr l/r freq-band))))
      ))
   ))

(defvar *start-seq* #(#xF5 #x33 #x3A #x64 #x57 #xC5 #x11 #xEC #x95 #x18 #x24 #xF6 #x77 #x02 #xCD #xAA))

(defun cresc-disconnected-beh ()
  (alambda
   ((:update arg-id val)
    (send fmt-println "~%arg: ~D val: ~D" arg-id val))
   
   ((:connect ip-addr port)
    (let ((sock (comm:open-tcp-stream ip-addr port
                                      :element-type '(unsigned-byte 8)
                                      :ipv6 nil)))
      (if sock
          (progn
            (write-sequence *start-seq* sock)
            (force-output sock)
            (become (cresc-connected-beh sock)))
        (error "No can do..."))))
   ))

(defun cresc-connected-beh (stream)
  (labels ((close-out ()
             (ignore-errors
               (close stream))
             (become (cresc-disconnected-beh))
             ))
    (alambda
     ((:update arg-id val)
      (handler-case
          (progn
            (write-sequence (list arg-id val) stream)
            (force-output stream))
        (error (c)
          (close-out)
          (error c))
        ))
     ((:reset)
      (let ((elev-pipe (sink-pipe (cresc-elev) self))
            (dh-pipe   (sink-pipe (cresc-dh) self)))
        (loop for ix from 0
              for el across (concatenate 'vector *elevl* *elevr*)
              do
              (send elev-pipe ix el))
        (loop for ix from 0
              for dh across (concatenate 'vector *dhl* *dhr*)
              do
              (send dh-pipe ix dh))
        ))
     ((:close)
      (close-out))
     )))

(defvar *cresc*  (create (cresc-disconnected-beh)))

(defun connect ()
  (send *cresc* :connect *host* *live-control-port*))

(defun disconnect ()
  (send *cresc* :close))

#|
(make-audiology-gui)
(connect)
(disconnect)

(send *cresc* :reset)
(send *cresc* :close)

(show-live)
(stop-live)
 |#
(defun filter-duplicates-beh (last-id last-val)
  (lambda (cust arg-id val)
    (unless (and (eql arg-id last-id)
                 (eql val    last-val))
      (become (filter-duplicates-beh arg-id val))
      (send cust arg-id val))))

(defun filter-duplicates ()
  (create (filter-duplicates-beh nil nil)))

(defun cresc-elev ()
  (actor
   (lambda (cust arg-id val)
     (send cust :update arg-id val))))

(defun cresc-dh ()
  (actor
   (lambda (cust arg-id val)
     (send cust :update (+ arg-id 20) (+ val 15)))))
  
(defun make-elev-plotter (pane)
  (let ((actor (create (lr-band-data-beh
                            (um:curry 'draw-elev-plot pane) :set-elev
                            (sink-pipe (filter-duplicates) (cresc-elev) *cresc*)
                            (copy-seq *elevl*)
                            (copy-seq *elevr*)
                            nil nil))))
    (send actor nil :show)
    actor))

(defun make-dh-plotter (pane)
  (let ((actor (create (lr-band-data-beh
                            (um:curry 'draw-dh-plot pane) :set-dh
                            (sink-pipe (filter-duplicates) (cresc-dh) *cresc*)
                            (copy-seq *dhl*)
                            (copy-seq *dhr*)
                            nil nil))))
    (send actor nil :show)
    actor))

;; ----------------------------------------------
;; Coordinated Left/Right selector buttons
;;
;; Clicking one chooses that channel, if not already selected,
;; deselecting the other if selected, or else deselecting the one if
;; was already selected.

(defun l/r-click (data intf)
  (with-slots (state left-select right-select l/r-callback) intf
    (case data
      (:LEFT
       (case state
         (:LEFT
          (setf state nil
                (capi:button-selected left-select) nil))
         (:RIGHT
          (setf state :LEFT
                (capi:button-selected right-select) nil))
         (t
          (setf state :LEFT))
         ))
      (:RIGHT
       (case state
         (:LEFT
          (setf state :RIGHT
                (capi:button-selected left-select) nil))
         (:RIGHT
          (setf state nil
                (capi:button-selected right-select) nil))
         (t
          (setf state :RIGHT))
         ))
      )
    (funcall l/r-callback)
    ))

(capi:define-interface <l/r-selector> ()
  ((state        :initform nil)
   (l/r-callback :initarg :l/r-callback))
  (:panes
   (left-select
    capi:radio-button
    :text "Left"
    :foreground :skyblue
    :data :LEFT
    :selection-callback 'l/r-click)
   (right-select
    capi:radio-button
    :text "Right"
    :foreground :red3
    :data :RIGHT
    :selection-callback 'l/r-click)
   )
  (:layouts
   (main-layout
    capi:row-layout
    '(left-select right-select))
   )
  (:default-initargs
   :layout       'main-layout
   :l/r-callback 'lw:do-nothing
   ))

#|
(capi:contain
 (make-instance '<l/r-selector>))
|#

;; ----------------------------------------------
;; Abstraction - combines slider with text-entry
;; They are kept in sync with each other.

(capi:define-interface <slider-with-readout> ()
  ((swr-title            :initarg :swr-title)
   (swr-title-position   :initarg :swr-title-position)
   (swr-message          :initarg :swr-message)
   (swr-size             :initarg :swr-size)
   (swr-orientation      :initarg :swr-orientation)
   (swr-start            :initarg :swr-start)
   (swr-end              :initarg :swr-end)
   (swr-foreground       :initarg :swr-foreground)
   (swr-background       :initarg :swr-background)
   (swr-tick-frequency   :initarg :swr-tick-frequency)
   (swr-callback         :initarg :swr-callback))
  (:panes
   (text-box
    capi:text-input-pane
    :title              swr-title
    :title-position     swr-title-position
    :text               "0"
    :enabled            t
    :foreground         swr-foreground
    :background         swr-background
    :callback           'swr-enter-value)
   (slider
    capi:slider
    :orientation        swr-orientation
    :visible-min-height swr-size
    :start              swr-start
    :end                swr-end
    :tick-frequency     swr-tick-frequency
    :start-point        (if (eql swr-orientation :vertical) :bottom :left)
    :slug-start         0
    :message            swr-message
    :callback           'swr-slider-changed)
   )
  (:layouts
   (main-layout
    capi:column-layout
    `(text-box slider)))
  (:default-initargs
   :layout             'main-layout
   :swr-title          "Slider with Readout"
   :swr-message        "0 .. 100"
   :swr-title-position :top
   :swr-orientation    :vertical
   :swr-size           200
   :swr-start            0
   :swr-end            100
   :swr-tick-frequency 1/10
   :background         :gray30
   :swr-foreground     :white
   :swr-background     :gray50
   :swr-callback       'lw:do-nothing))
   
(defun swr-enter-value (txt intf)
  (let ((val (ignore-errors
               (read-from-string txt))))
    (with-slots (swr-start swr-end slider text-box swr-callback) intf
      (cond ((and (integerp val)
                  (<= swr-start val swr-end))
             (setf (capi:range-slug-start slider) val)
             (funcall swr-callback val))

          (t
           (setf (capi:text-input-pane-text text-box)
                 (format nil "~D" (capi:range-slug-start slider)))
           (capi:display-message-for-pane text-box
                                          "Please enter an integer between ~D and ~D"
                                          swr-start swr-end))
          ))))

(defun swr-slider-changed (pane val kind)
  (when (eql kind :DRAG)
    (let ((intf (capi:element-interface pane)))
      (with-slots (text-box swr-callback) intf
        (setf (capi:text-input-pane-text text-box) (format nil "~D" val))
        (funcall swr-callback val)
        ))))

(defun swr-set-slider (intf val)
  (capi:execute-with-interface
   intf
   (lambda ()
     (with-slots (slider text-box) intf
       (setf (capi:range-slug-start slider)       val
             (capi:text-input-pane-text text-box) (format nil "~D" val))))
   ))

;; --------------------------------------------------------------------
;; The band (side, freq) selectors

(defvar *freq-bands* '(250 500 750 1k 1.5k 2k 3k 4k 6k 8k))

(defun choose-audiology-freq-band (sel intf)
  (with-slots (shadow-chan) intf
    (setf shadow-chan (position sel *freq-bands*))
    (notify-actors-of-selection intf)))

(defun notify-actors-of-selection (intf)
  ;; Initial code executes in CAPI thread during a callback
  (with-slots (shadow-chan
               l/r-sel
               elev-plotter
               dh-plotter
               elev-slider
               dh-slider) intf
    (beta msg
        (send-to-all (list elev-plotter dh-plotter)
                     beta :select (slot-value l/r-sel 'state) shadow-chan)
      ;; ... but here, we are operating in Actor land
      (match msg
        ((:set-elev val)
         (swr-set-slider elev-slider val))
        ((:set-dh val)
         (swr-set-slider dh-slider val))
        ))
    ))

(capi:define-interface <audiology-panel> ()
  ((shadow-chan  :initform 0)
   (elev-plotter :accessor elev-plotter  :initform nil)
   (dh-plotter   :accessor dh-plotter    :initform nil))
  (:panes
   ;; threshold elevation adjustment
   (elev-slider
    <slider-with-readout>
    :swr-title          "Elev dBHL"
    :swr-size           180
    :swr-start            0
    :swr-end             90
    :swr-tick-frequency 1/9
    :swr-message        "0 .. 90 dB"
    :swr-callback       (lambda (val)
                          (send elev-plotter sink :update val)))

   ;; hyper-recruitment / decruitment adjustment
   (dh-slider
    <slider-with-readout>
    :swr-title          "+D/-H dB"
    :swr-size           180
    :swr-start          -15
    :swr-end             15
    :swr-tick-frequency 1/10
    :swr-message        "-15 .. 15 dB"
    :swr-callback       (lambda (val)
                          (send dh-plotter sink :update val)))

   ;; left / right selection
   (l/r-sel
    <l/r-selector>
    :l/r-callback       (um:curry 'notify-actors-of-selection capi:interface))

   ;; frequency band selection
   (chan-sel
    capi:option-pane
    :items              *freq-bands*
    :selected-item      250
    :selection-callback 'choose-audiology-freq-band)

   (thresh-plot plt:articulated-plotter-pane
                :accessor thresh-plot
                :visible-min-width  400
                :visible-min-height 300)
   
   (dh-plot     plt:articulated-plotter-pane
                :accessor dh-plot
                :visible-min-width  400
                :visible-min-height 300)
   )
  (:layouts
   (graph-panes
    Capi:column-layout
    '(thresh-plot dh-plot))
   (row-top
    capi:row-layout
    '(l/r-sel chan-sel))
   (row-bot
    capi:row-layout
    '(elev-slider dh-slider))
   (ctrls-layout
    capi:column-layout
    '(row-top row-bot))
   (mid-ctrls-layout
    capi:column-layout
    '(nil ctrls-layout nil))
   (main-layout
    capi:row-layout
    '(graph-panes mid-ctrls-layout))
   )
  (:default-initargs
   :layout 'main-layout
   :title  "Audiology Adjustments"
   ))

(defmethod initialize-instance :after ((obj <audiology-panel>) &key &allow-other-keys)
  (setf (elev-plotter obj) (make-elev-plotter (thresh-plot obj))
        (dh-plotter   obj) (make-dh-plotter   (dh-plot     obj))))

(defun make-audiology-gui ()
  (capi:display (make-instance '<audiology-panel>
                               :title (format nil "Audiology Adjustments: ~A" *host*))
                ))

#|
(make-audiology-gui)
|#
;; -----------------------------------------------------------
#|
(with-open-stream (http (comm:open-tcp-stream 
                         "localhost" 1337))
  #||#
  (format http "GET / HTTP/1.0~C~C~C~C"
               (code-char 13) (code-char 10)
               (code-char 13) (code-char 10))
  (force-output http)
  (write-string "Waiting for reply...")
  #||#
  (loop for ch = (read-char-no-hang http nil :eof)
        until ch
        do (write-char #\.)
           (sleep 0.25)
        finally (unless (eq ch :eof)
                  (unread-char ch http))) 
  (terpri)
  (loop for line = (read-line http nil nil)
        while line
        do (write-line line)))

(setf sock (comm:open-tcp-stream "localhost" 65100))
(close sock)
(connect)
|#
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

;; (defvar *graph-ver* 0)

(defun show-live ()
  ;; (incf *graph-ver*)
  (mp:process-run-function 'live () 'live-view))

(defun stop-live ()
  (setf *done* t))

#|
(show-live)
(stop-live)
|#

#|
(defun ovly (arr nel grp)
  (make-instance 'um:ovly-vec
                 :arr arr
                 :nel nel
                 :off (* grp nel)
                 :element-type 'single-float))

(defun live-view ()
  (let* ((xarr   (sys:make-typed-aref-vector #.(* 4 12 4)))
         (specl  (ovly xarr 12 0))
         (gainl  (ovly xarr 12 1))
         (specr  (ovly xarr 12 2))
         (gainr  (ovly xarr 12 3))
         (ctr    0)
         (start  (get-universal-time)))
    (declare (dynamic-extent xarr))
    (unwind-protect
        (block #1=processing
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
               (loop for ix from 0 below #.(* 4 12 4) do
                       (let ((byt  (read-byte sock nil sock)))
                         (when (eql byt sock)
                           (print "Connection timeout.")
                           (return-from #1#))
                         (setf (sys:typed-aref '(unsigned-byte 8) xarr ix) byt)
                         ))
               
               (ask dual-graphs
                    (um:user-defined-sequence-to-vector specl)
                    (um:user-defined-sequence-to-vector specr)
                    (um:user-defined-sequence-to-vector gainl)
                    (um:user-defined-sequence-to-vector gainr))
               (when *done*
                 (return-from #1#))
               (when (and (< ctr 100)
                          (= (incf ctr) 100))
                 (format t "~%Rate: ~,1F" (/ 1f2 (max 0.01f0 (- (get-universal-time) start)))))
               )))
      (print "Live view exiting.")
      (setf *done* nil))
    ))
|#

