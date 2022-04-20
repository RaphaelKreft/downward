# Collection of Dev Thoughts related to BA-Thesis

## Domain Abstraction

basically just need to store equivalence relations for every variable of Task.. -> Hashmap? {var1: [(a,b),(a,a),...],
var2: ...} suffices since just have discrete states? Currently in domain_abstracted_task:
- Map is nested vectors ->

## Implementation

-Have one Interface Heuristic that can be given options to use different "heuristics" == Construction Algos we will
implement

## CEGAR Algorithm v1

- just one heuristic not many like in PDB -> consider this maybe in later versions -> check diminishing returns
- Input: Original Task Output: generated heuristic -> map of values: {state in domain abstraction: heuristic value to
  return}

- Two types of flaws:
    1. precondition violation flaws: Abstraction can lead to deletion of preconditions for actions in state space
       induced by abstraction
    2. goal flaws: States might get merged and not agree to be a goal state -> landing in non-goal state
- general algorithm CEGAR:
  # INIT

Init:
- read from goal state the value variables must have - split domain of every variable to -> {val in goal} and {rest}

Trace: sequence of pairs containing operator and goal Solution: just sequence of states?

