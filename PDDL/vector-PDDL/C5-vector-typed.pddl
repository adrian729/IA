(define (domain vectorTyped)
  (:requirements :strips :typing)
  (:types pos valor - object)

  (:predicates (val ?x - pos ?vx - valor)
               (next ?x - pos ?y -pos))

  (:action swap
    :parameters (?x - pos ?y - pos ?vx - valor ?vy - valor)
    :precondition (and (val ?x ?vx) (val ?y ?vy) (next ?x ?y))
    :effect (and (val ?y ?vx) (val ?x ?vy) (not (val ?x ?vx)) (not (val ?y ?vy))
            )
  )
)