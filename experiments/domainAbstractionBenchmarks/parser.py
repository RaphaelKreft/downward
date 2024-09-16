#! usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern("Num AbstractStates", r"#Abstract States: (\d+)", type=int)
parser.add_pattern("Num CEGAR Loop Iterations", r"#CEGAR Loop Iterations: (\d+)", type=int)
parser.add_pattern("Precalculation-Time", r"Time for precalculation of heuristic-values: (.+)s",type=float)
parser.parse()
