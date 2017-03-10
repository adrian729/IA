(define (problem vector1) (:domain vector)
(:objects
	p1 p2 p3 a b c
)
(:init
  (next p1 p2)
  (next p2 p3)
  (val p1 a)
  (val p2 b)
  (val p3 c)
)
(:goal (and
  (val p1 c)
  (val p2 b)
  (val p3 a)
	)
))

