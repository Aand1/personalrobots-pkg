(in-package :vb-node)

(define-debug-topic :sequence-node :vb-node)

(defclass <sequence-node> (<node>)
  ((action-sequence :reader action-sequence :writer set-action-sequence)
   (status :initform :initial)))

;; We don't have an initialize-instance :after method to add the node's variables
;; This is because they depend on what the refinement is, and we can't compute this until
;; we know what the initial valuation is.  So initialization is done in initialize-sequence-node.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The node's outputs are computed from the node's 
;; own progressed/regressed valuations together with the
;; final/initial child's output valuation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmethod optimistic-progressor ((n <sequence-node>))
  (make-simple-alist-updater (my-progressed-optimistic children-progressed-optimistic)
    (pointwise-min-upper-bound my-progressed-optimistic children-progressed-optimistic)))

(defmethod pessimistic-progressor ((n <sequence-node>))
  (make-simple-alist-updater (my-progressed-pessimistic children-progressed-pessimistic)
    (pointwise-max-lower-bound my-progressed-pessimistic children-progressed-pessimistic)))


(defmethod optimistic-regressor ((n <sequence-node>))
  (make-simple-alist-updater (my-regressed-optimistic children-regressed-optimistic)
    (pointwise-min-upper-bound my-regressed-optimistic children-regressed-optimistic)))

(defmethod pessimistic-regressor ((n <sequence-node>))
  (make-simple-alist-updater (my-regressed-pessimistic children-regressed-pessimistic)
    (pointwise-max-lower-bound my-regressed-pessimistic children-regressed-pessimistic)))


 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Adding children
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(defmethod add-child :after ((n <sequence-node>) i new-node-type &rest args)
  (declare (ignore new-node-type args))

  (let ((child (child i n)))
    ;; Output variables already created in initialize-instance, so just tie them
    (dolist (vars '((progressed-optimistic child-progressed-optimistic) (progressed-pessimistic child-progressed-pessimistic)
		    (regressed-optimistic child-regressed-optimistic) (regressed-pessimistic child-regressed-pessimistic)))
      (tie-variables child (first vars) n (cons (second vars) i)))
    

    ;; Forward input messages
    (cond 
      ((zerop i)
       (tie-variables n 'initial-optimistic child 'initial-optimistic)
       (tie-variables n 'initial-pessimistic child 'initial-pessimistic))
      (t
       (tie-variables n (cons 'child-progressed-optimistic (1- i)) child 'initial-optimistic)
       (tie-variables n (cons 'child-progressed-pessimistic (1- i)) child 'initial-pessimistic)))

    ;; Backward input messages
    (cond
      ((= i (1- (sequence-length n)))
       (tie-variables n 'final-optimistic child 'final-optimistic)
       (tie-variables n 'final-pessimistic child 'final-pessimistic))
      (t
       (tie-variables n (cons 'child-regressed-optimistic (1+ i)) child 'final-optimistic)
       (tie-variables n (cons 'child-regressed-pessimistic (1+ i)) child 'final-pessimistic)))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Compute cycle: update self, then pass control to child
;; with worst pessimistic estimate
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defmethod compute-cycle ((n <sequence-node>))
  (debug-out :sequence-node 1 t "~&In compute cycle of sequence-node ~a with status ~a" (action n) (status n))
  (ecase (status n)
    (:initial (initialize-sequence-node n) (setf (status n) :ready))
    (:ready 
       (do-all-updates n)
       (let ((i (maximizing-element (child-ids n) (sequence-node-child-evaluator n))))
	 (compute-cycle (child i n)))
       (do-all-updates n))))

(defun sequence-node-child-evaluator (n)
  (declare (ignore n))
  #'(lambda (i)
      (format t "~&Stub: Enter value for child ~a of sequence node: " i)
      (read)))


(defun initialize-sequence-node (n)
  (set-action-sequence (item 0 (refinements (action n) (hierarchy n) :init-opt-set (reachable-set (current-value n 'initial-optimistic)))) n)

  (let ((h (hierarchy n))
	(ref (action-sequence n))
	(l (sequence-length n)))

    ;; Add child output variables
    ;; They have dependants within the node only at the ends
    ;; We have to add all these variables before adding the child nodes because of cyclic dependencies
    (dotimes (i l)
      (add-variable n (cons 'child-progressed-optimistic i) :external :initial-value (make-simple-valuation (universal-set (planning-domain n)) 'infty)
		    :dependants (when (= i (1- l)) '(progressed-optimistic)))
      (add-variable n (cons 'child-progressed-pessimistic i) :external :initial-value (make-simple-valuation (empty-set (planning-domain n)) '-infty)
		    :dependants (when (= i (1- l)) '(progressed-pessimistic)))
      (add-variable n (cons 'child-regressed-optimistic i) :external :initial-value (make-simple-valuation (universal-set (planning-domain n)) 'infty)
		    :dependants (when (zerop i) '(regressed-optimistic)))
      (add-variable n (cons 'child-regressed-pessimistic i) :external :initial-value (make-simple-valuation (empty-set (planning-domain n)) '-infty)
		    :dependants (when (zerop i) '(regressed-pessimistic))))

    ;; Add the child nodes themselves
    (do-elements (child-action ref nil i)
      (create-child-for-action h n i child-action))))

(defmethod action-node-type ((c (eql ':sequence)))
  '<sequence-node>)

(defun sequence-length (n)
  (length (action-sequence n)))
