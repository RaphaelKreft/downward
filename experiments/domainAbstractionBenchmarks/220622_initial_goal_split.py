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


REVISION = "118db8a01e47d51f996ba520a5c0dd9a8c82f7e3"
REPO = os.environ["DOWNWARD_REPO"]
BENCHMARKS_DIR, SUITE, ENVIRONMENT = setup_environment()

# time in seconds
max_state_caps = [256, 512, 1024, 2048, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000]
CONFIGS = []

for state_cap in max_state_caps:
    CONFIGS.append(common_setup.IssueConfig(f"daPrecomp-{state_cap}-gs", ["--search", f"astar(domain_abstraction(precalculation=true, max_states={state_cap},initial_goal_split=true))"]))
    CONFIGS.append(common_setup.IssueConfig(f"daOTF-{state_cap}-gs", ["--search", f"astar(domain_abstraction(precalculation=false, max_states={state_cap},initial_goal_split=true))"]))

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

exp.add_absolute_report_step(attributes=(["Num AbstractStates", "Num CEGAR Loop Iterations", "Precalculation-Time"] + common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES))
#exp.add_onerev_scatter_plot_step()
exp.add_parse_again_step()

exp.run_steps()
