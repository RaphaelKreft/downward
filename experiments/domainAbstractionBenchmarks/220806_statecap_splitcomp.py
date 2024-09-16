#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

import common_setup
from downward.reports.absolute import AbsoluteReport

from lab.reports import Attribute, arithmetic_mean
from lab.environments import LocalEnvironment, BaselSlurmEnvironment


def setup_environment():
    if common_setup.is_running_on_cluster():
        benchmarks_dir = os.environ["DOWNWARD_BENCHMARKS"]
        suite = common_setup.DEFAULT_OPTIMAL_SUITE
        #suite = ["gripper:prob01.pddl", "gripper:prob02.pddl", "miconic:s1-0.pddl"]
        environment = BaselSlurmEnvironment(
            partition=common_setup.PARTITION,
            email="r.kreft@unibas.ch",
            export=["DOWNWARD_BENCHMARKS"],
            setup="export PATH=" + ":".join([
                "/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin",
                "/scicore/soft/apps/GCCcore/8.3.0/bin",
                "/export/soft/lua_lmod/centos7/lmod/lmod/libexec",
                "$PATH",
            ]) + "\nexport LD_LIBRARY_PATH=" + ":".join([
                "/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib",
                "/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib",
                "/scicore/soft/apps/GCCcore/8.3.0/lib64",
                "/scicore/soft/apps/GCCcore/8.3.0/lib",
            ]),
        )
    else:
        benchmarks_dir = os.environ["DOWNWARD_BENCHMARKS"]
        suite = ["gripper:prob01.pddl", "gripper:prob02.pddl", "miconic:s1-0.pddl"]
        environment = LocalEnvironment(processes=2)

    return benchmarks_dir, suite, environment


REVISION = "de12cf30bfc7eb5efabe6f6c8d603eb8f1ac3752"
REPO = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR, SUITE, ENVIRONMENT = setup_environment()

# time in seconds
max_state_caps = [2048,500]
CONFIGS = []

for state_cap in max_state_caps:
    CONFIGS.append(common_setup.IssueConfig(f"daPrecomp-{state_cap}", ["--search", f"astar(domain_abstraction(precalculation=true, max_states={state_cap}))"]))
    CONFIGS.append(common_setup.IssueConfig(f"daPrecomp-{state_cap}-sv", ["--search", f"astar(domain_abstraction(precalculation=true, max_states={state_cap},singlevaluesplit=true))"]))

exp = common_setup.IssueExperiment(
    revisions=[REVISION],
    configs=CONFIGS,
    environment=ENVIRONMENT,
   )

exp.add_suite(BENCHMARKS_DIR, SUITE)

exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser("parser.py")

exp.add_step("build", exp.build)
exp.add_step("start", exp.start_runs)
exp.add_fetcher(name="fetch")

exp.add_absolute_report_step(attributes=(["Num AbstractStates", "Num CEGAR Loop Iterations", "Precalc time"] + common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES))
exp.add_parse_again_step()

plot_pairs = [("daPrecomp-2048","daPrecomp-2048-sv"),("daPrecomp-5000","daPrecomp-5000-sv"),("daPrecomp-2048","daPrecomp-5000")]
for alg1,alg2 in plot_pairs:
    exp.add_report(ScatterPlotReport(attributes=["expansions_until_last_jump"],filter_algorithm=[alg1,alg2]))
    exp.add_report(ScatterPlotReport(attributes=["initial_h_value"],filter_algorithm=[alg1,alg2]))
    exp.add_report(ScatterPlotReport(attributes=["total_time"],filter_algorithm=[alg1,alg2]))

exp.run_steps()
