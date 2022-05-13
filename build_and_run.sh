sudo rm -r builds
./build.py debug
./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction())"
