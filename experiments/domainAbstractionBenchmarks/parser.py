#! usr/bin/env python

from lab.parser import Parser

parser = Parser()
parser.add_pattern("#AbstractStates", r"#Abstract States: (\d+)", type=int)
parser.add_pattern("#CEGAR Loop Iterations", r"#CEGAR Loop Iterations: (\d+)", type=int)

parser.parse()
