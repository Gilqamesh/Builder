(define (caar expr) (car (car expr)))
(define (cdar expr) (cdr (car expr)))
(define (cadr expr) (car (cdr expr)))
(define (cddr expr) (cdr (cdr expr)))

(define (not expr)
  (if expr
      #f
      #t))

(define (map f l)
  (if (eq? l '())
      '()
      (cons (f (car l)) (map f (cdr l)))))

(define (list-tail l n)
  (if (= n 0)
      l
      (list-tail (cdr l) (- n 1))))

(define (list-ref l n)
  (car (list-tail l n)))

#|
  (let ((x 10) (y 20)) (+ x y)) -> ((lambda (x y) (+ x y)) 10 20)
  (let f ((x 10) (y 20)) (f x y)) -> (let ((f (lambda (x y) (+ x y))))
                                          (let ((x 10) (y 20))
                                               (f x y)))
                                  -> ((lambda (f) ((lambda (x y) (f x y)) 10 20)) (lambda (x y) (f x y)))
(let fib-iter ((a 1)
              (b 0)
              (count n))
     (if (= count 0)
         b
         (fib-iter (+ a b) a (- count 1))))
(let ((fib-iter (lambda (a b count) 

(let* ((a 10) (b (+ a 20)) (c (+ b a)))
      (+ a b c))

((lambda (a) ((lambda (b) ((lambda (c) (+ a b c)) (+ a 20))) (+ a 20))) 10)

|#
(defmacro let (var-val-pair-list . body-list)
  `((lambda ,(map (lambda (e) (car e)) var-val-pair-list) ,@body-list) ,@(map (lambda (e) (cadr e)) var-val-pair-list)))

(defmacro let* (var-val-pair-list . body-list)
  (define (let*-helper pair-list)
    (if (and pair-list (cdr pair-list))
        `((lambda (,(caar pair-list)) ,(let*-helper (cdr pair-list))) ,(list-ref (car pair-list) 1))
        `((lambda (,(caar pair-list)) ,@body-list) ,(list-ref (car pair-list) 1))))
  (let*-helper var-val-pair-list))

(defmacro cond (. cond-cons-pairs)
  (define (cond-helper rem-cond-cons-pairs)
    (when rem-cond-cons-pairs
      (if (eq? (caar rem-cond-cons-pairs) 'else)
          `(begin ,@(cdar rem-cond-cons-pairs))
          (if (eq? '=> (list-ref (car rem-cond-cons-pairs) 1)) #| Exercise 4.5 |#
              (let ((cond-val (list-ref (car rem-cond-cons-pairs) 0)))
                   `(if ,cond-val
                        (apply ,(list-ref (car rem-cond-cons-pairs) 2) (,cond-val))
                        ,(cond-helper (cdr rem-cond-cons-pairs))))
              `(if ,(caar rem-cond-cons-pairs)
                   (begin ,@(cdar rem-cond-cons-pairs))
                   ,(cond-helper (cdr rem-cond-cons-pairs)))))))
  (cond-helper cond-cons-pairs))

(defmacro when (condition . consequence)
  `(if ,condition (begin ,@consequence)))


(define (list-len l)
  (if (nil? l)
      0
      (+ 1 (list-len (cdr l)))))

(define list_of_len1 (list (if '() #t) 1)) 

