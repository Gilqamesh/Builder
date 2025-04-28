(define (caar expr) (car (car expr)))
(define (cdar expr) (cdr (car expr)))
(define (cadr expr) (car (cdr expr)))
(define (cddr expr) (cdr (cdr expr)))

(define (not expr)
  (if expr
      #f
      #t))

(define (and expr . rest)
  (if (not expr)
      #f
      (if (nil? rest)
          #t
          (apply and (cons (car rest) (cdr rest))))))

(define (or expr . rest)
  (if expr
      #t
      (if (nil? rest)
          #f
          (apply or (cons (car rest) (cdr rest)))))) 

(define (map f l)
  (if (eq? l '())
      '()
      (cons (f (car l)) (map f (cdr l)))))

(defmacro let (var-val-pair-list . body-list)
  `((lambda ,(map (lambda (e) (car e)) var-val-pair-list) ,@body-list) ,@(map (lambda (e) (cadr e)) var-val-pair-list)))

(defmacro cond (. cond-cons-pairs)
  (define (cond-helper rem-cond-cons-pairs)
    (when rem-cond-cons-pairs
      (if (eq? (caar rem-cond-cons-pairs) 'else)
          `(begin ,@(cdar rem-cond-cons-pairs))
          `(if ,(caar rem-cond-cons-pairs)
               (begin ,@(cdar rem-cond-cons-pairs))
               ,(cond-helper (cdr rem-cond-cons-pairs))))))
  (cond-helper cond-cons-pairs))

(defmacro when (condition . consequence)
  `(if ,condition (begin ,@consequence)))


(define (list-len l)
  (if (nil? l)
      0
      (+ 1 (list-len (cdr l)))))

(define list_of_len1 (list (if '() #t) 1)) 

