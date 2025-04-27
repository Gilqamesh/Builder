#| (defmacro when (condition consequence)
  (list 'if condition consequence)) |#

(define (map f l)
  (if (eq? l '())
      '()
      (cons (f (car l)) (map f (cdr l)))))

(defmacro when (condition . consequence)
  `(if ,condition (begin ,@consequence)))

#| (cond ((condition consequence)
          (condition consequence)
          (else consequence))) |#

(define (square x) (* x x))

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

#| (let ((var1 val1) (var2 val3) ...) body)
  -> ((lambda (var1 var2) body) val1 val2)
|#

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

(define (list-len l)
  (if (nil? l)
      0
      (+ 1 (list-len (cdr l)))))

#| todo: compile out void exprs |#
(define list_of_len1 (list (if '() #t) 1)) 

#| testing variadic parameters
 (define (acc l r)
  (if (nil? l)
      r
      (acc (cdr l) (+ (car l) r)))) #| |#
 
(define a ((lambda (x y . z) (+ x y (acc z 0))) 1 2 3 4 5 6 7 8 9 10)) |#

#| 
(define (fib n) 
  (cond ((= n 0) 1)
        ((= n 1) 1)
        (#t (fib (- n 1) (- n 2)))))
|#
