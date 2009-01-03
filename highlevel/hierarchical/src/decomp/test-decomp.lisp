(defpackage :test-decomp
  (:use :lookahead
	:decomp
	:utils
	:blocks
	:cl
	:decomp
	:mapping
	:env-user
	:dependency-graph
	:set
	:vb-node
	:two-by-n
	:prop-logic)
  (:import-from
   :vb-node
   :children
   :child
   
   :children-progressed-optimistic
   :children-progressed-pessimistic
   :children-regressed-optimistic
   :children-regressed-pessimistic
   :child-progressed-optimistic
   :child-progressed-pessimistic
   :child-regressed-optimistic
   :child-regressed-pessimistic

   :status)
  )

(in-package :test-decomp)


(load-relative #P"dependency/test-dependency.lisp")




;; two-by-n 
(defparameter costs #3A(((1 0) (1 6) (7 1)) ((0 0) (8 4) (6 5))))
(defparameter e (make-instance '<two-by-n> :costs costs))
(defparameter h (make-instance '<two-by-n-hierarchy> :planning-domain e))
(defparameter descs (make-instance '<two-by-n-descriptions> :hierarchy h))
(setq *progress-optimistic-counts* 0 *progress-pessimistic-counts* 0 *regress-optimistic-counts* 0 *regress-pessimistic-counts* 0)

(tests "two-by-n environment"
  ((find-satisficing-plan descs '-infty) #(0 0 0))
  ((< (max *progress-optimistic-counts* *progress-pessimistic-counts*  *regress-optimistic-counts* *regress-pessimistic-counts*) 15) t)
  ((find-optimal-plan descs) #(0 0 1))
  ((< (max *progress-optimistic-counts* *progress-pessimistic-counts*  *regress-optimistic-counts* *regress-pessimistic-counts*) 60) t))


    