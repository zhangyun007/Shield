(quote ...)

(class point (list (list n "hello!!!!!")(list m (lambda x (* 2 x)))))
(define x (point))
(print (. x n))
(procedure? (. x m))
(quote here are bugs)
(. x m)


