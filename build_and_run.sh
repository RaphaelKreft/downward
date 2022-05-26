#!/bin/bash

./build.py debug

if [ "$1" -eq 0 ]
then
  if [ "$2" -eq 1 ]
  then
    ./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction(precalculation=true))"
  else
    ./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction())"
  fi
else
  if [ "$2" -eq 1 ]
    then
      ./fast-downward.py --debug ../downward-benchmarks/transport-opt08-strips/p01.pddl --search "astar(domain_abstraction(precalculation=true))"
    else
      ./fast-downward.py --debug ../downward-benchmarks/transport-opt08-strips/p01.pddl --search "astar(domain_abstraction())"
  fi
fi
