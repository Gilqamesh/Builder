(defmacro when (condition consequence)
  (list 'if condition consequence))

(define (map f l)
  (if (eq? l '())
      '()
      (cons (f (car l)) (map f (cdr l)))))

