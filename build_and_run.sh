#!/bin/bash

./build.py debug

if [ "$1" -eq 0 ]
then
  if [ "$2" -eq 1 ]
  then
    ./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction(split_method=EvenSplit))"
  else
    ./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction())"
  fi
else
  if [ "$2" -eq 1 ]

    then
      ./fast-downward.py --debug ../downward-benchmarks/transport-opt08-strips/p01.pddl --search "astar(domain_abstraction(split_method=EvenSplit))"
      echo "ran hard problem with even Split"
    else
      ./fast-downward.py --debug ../downward-benchmarks/transport-opt08-strips/p01.pddl --search "astar(domain_abstraction())"
      echo "Ran hard problem with hardsplit!"
  fi
fi
