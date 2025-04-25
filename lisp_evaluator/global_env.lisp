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

(defmacro cond (. cond-cons-pairs)
  (define (mymacro-helper rem-cond-cons-pairs)
    (if (cons? rem-cond-cons-pairs)
        `(if ,(caar rem-cond-cons-pairs)
             ,(car (cdar rem-cond-cons-pairs))
             ,(mymacro-helper (cdr rem-cond-cons-pairs)))))
  (mymacro-helper cond-cons-pairs))

#| testing variadic parameters
 (define (acc l r)
  (if (nil? l)
      r
      (acc (cdr l) (+ (car l) r)))) #| |#
 
(define a ((lambda (x y . z) (+ x y (acc z 0))) 1 2 3 4 5 6 7 8 9 10)) |#


