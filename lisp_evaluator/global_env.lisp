#| (defmacro when (condition consequence)
  (list 'if condition consequence)) |#

(define (map f l)
  (if (eq? l '())
      '()
      (cons (f (car l)) (map f (cdr l)))))

(defmacro when (condition consequence)
  `(if ,condition (begin ,@consequence)))

#| (cond ((condition consequence)
          (condition consequence)
          (else consequence))) |#
(defmacro cond (cond-body)
  (lambda ()
    (define a 3)
    a))

#| testing variadic parameters
 (define (acc l r)
  (if (nil? l)
      r
      (acc (cdr l) (+ (car l) r)))) #| |#
 
(define a ((lambda (x y . z) (+ x y (acc z 0))) 1 2 3 4 5 6 7 8 9 10)) |#


