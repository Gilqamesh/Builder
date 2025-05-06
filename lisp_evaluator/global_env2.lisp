(define quote (macro (e) e))
(define nil '())
(define nil? (lambda (e) (eq? e nil)))
(define list (lambda (. e) e))
(define map (lambda (f l)
  (if (nil? l)
    nil
    (cons (f (car l)) (map f (cdr l))))))
(define begin (lambda (. e)
  (define begin-helper (lambda (rem prev)
    (if (nil? rem)
        prev
        (begin-helper (cdr rem) (eval (car rem))))))
  (begin-helper e nil)))

(define filter (lambda (f l)
  (if (nil? l)
      nil
      (if (f (car l))
          (cons (f (car l)) (filter f (cdr l)))
          (filter f (cdr l))))))

(define a (macro (. e)
  (define b (macro (e)
    (define b-helper (lambda (e)
      (if (nil? e)
          nil
          (cons (+ 1 (car e)) (b-helper (cdr e))))))
    (b-helper e)))
  (b ,e)))

(define quasiquote (macro (e)
  (define quasiquote-helper (lambda (e )))
  (quasiquote-helper e))

  (if (cons? e)
      (if (eq? (car e) 'unquote)
          (car (cdr e))
          (if (eq? (car e) 'unquote-splicing)
              (list 'multiple-values (cdr e)

