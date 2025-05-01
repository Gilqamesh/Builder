(define (caar expr) (car (car expr)))
(define (cdar expr) (cdr (car expr)))
(define (cadr expr) (car (cdr expr)))
(define (cddr expr) (cdr (cdr expr)))

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
(let fib-iter ((a 1)
              (b 0)
              (count n))
     (if (= count 0)
         b
         (fib-iter (+ a b) a (- count 1))))

((lambda ()
  (define (fib-iter a b count)
    (if (= count 0)
        b
        (fib-iter (+ a b) a (- count 1))))
  (let ((a 1)
        (b 0)
        (count n))
       (if (= count 0)
           b
           (fib-iter (+ a b) a (- count 1))))))

|#

(defreader ";"
           (lambda (is)
             (define (read-until)
               (if (and (not (is-at-end? is))
                        (not (eq-char? (read-char is) #\newline)))
                   (read-until)))
             (read-until)))

(defmacro let (var-val-pair-list . body-list)
  (if (symbol? var-val-pair-list) ; Ex 4.8
      `((lambda ()
        (define (,var-val-pair-list ,@(map (lambda (e) (car e)) (car body-list))) ,@(cdr body-list))
        (let ,(car body-list) ,@(cdr body-list))))
      `((lambda ,(map (lambda (e) (car e)) var-val-pair-list) ,@body-list) ,@(map (lambda (e) (cadr e)) var-val-pair-list))))

(defmacro let* (var-val-pair-list . body-list)
  (define (let*-helper pair-list)
    (if (and pair-list (cdr pair-list))
        `((lambda (,(caar pair-list)) ,(let*-helper (cdr pair-list))) ,(list-ref (car pair-list) 1))
        `((lambda (,(caar pair-list)) ,@body-list) ,(list-ref (car pair-list) 1))))
  (let*-helper var-val-pair-list))

; example usage: (cond ((eq? x 10) (- x 10))
;                      ((< x 30) (* x -1))
;                      (42 => (lambda (e) (display e) (newline e)))
;                      ('(1 2 3) => car))
(defmacro cond (. cond-cons-pairs)
  (define (cond-helper rem-cond-cons-pairs)
    (when rem-cond-cons-pairs
      (if (eq? (caar rem-cond-cons-pairs) 'else)
          `(begin ,@(cdar rem-cond-cons-pairs))
          (if (eq? '=> (list-ref (car rem-cond-cons-pairs) 1)) ; Ex 4.5
              (let ((condition (list-ref (car rem-cond-cons-pairs) 0))
                    (f (list-ref (car rem-cond-cons-pairs) 2)))
                   `(if ,condition
                        (apply ,f (,(eval condition)))
                        ,(cond-helper (cdr rem-cond-cons-pairs))))
              `(if ,(caar rem-cond-cons-pairs)
                   (begin ,@(cdar rem-cond-cons-pairs))
                   ,(cond-helper (cdr rem-cond-cons-pairs)))))))
  (cond-helper cond-cons-pairs))

(defmacro when (condition . consequence)
  ; example usage: (when (< 0 i)
  ;                  (display i) (newline)
  ;                  i)
  `(if ,condition (begin ,@consequence)))

(define (iter start f-get f-apply f-next f-cond)
  (when (f-cond start)
        (f-apply (f-get start))
        (iter (f-next start) f-get f-apply f-next f-cond)))

(define (identity x) x)

(defmacro for (variable . rest) ; Ex 4.9
  (cond ((eq? 'in (car rest))
         (let ((elements (list-ref rest 1)))
              (if (and (cons? (list-ref rest 2))
                       (eq? 'while (car (list-ref rest 2))))
                  ; example usage: (for x in '(1 2 3)
                  ;                  (while (< x 4))
                  ;                  (display x) (newline))
                  `(iter ,elements
                         car
                         (lambda (,variable) ,@(list-tail rest 3))
                         cdr
                         (lambda (x) (and x (let ((,variable (car x))) ,(cadr (list-ref rest 2))))))
                  ; example usage: (for x in '(1 2 3)
                  ;                  (display x) (newline))
                  `(iter ,elements
                         car
                         (lambda (,variable) ,@(list-tail rest 2))
                         cdr
                         identity))))
        ((eq? 'from (car rest))
         (let ((start (list-ref rest 1))
               (end (list-ref rest 3)))
              (if (and (cons? (list-ref rest 4)) (eq? 'while (car (list-ref rest 4))))
                  ; example usage: (for x from 1 to 5
                  ;                  (while (< x 4))
                  ;                  (display x) (newline))
                  `(iter ,start
                         identity
                         (lambda (,variable) ,@(list-tail rest 5))
                         (lambda (e) (+ 1 e))
                         (lambda (,variable)
                                 (and (< ,variable ,(+ 1 end))
                                      ,(cadr (list-ref rest 4)))))
                  ; example usage: (for x from 1 to 5
                  ;                  (display x) (newline))
                  `(iter ,start
                         identity
                         (lambda (,variable) ,@(list-tail rest 4))
                         (lambda (e) (+ 1 e))
                         (lambda (e) (< e ,(+ 1 end)))))))
        (else (throw `("for: incorrect form" ("variable" ,variable) ("rest" ,rest))))))

(define (list-len l)
  (if (nil? l)
      0
      (+ 1 (list-len (cdr l)))))

(define list_of_len1 (list (if '() #t) 1)) 

