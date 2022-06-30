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

CONFIGS = []
# configs
CONFIGS.append(common_setup.IssueConfig(f"daPrecomp-1024-gs", ["--search", "astar(domain_abstraction(precalculation=true, max_states=1024,initial_goal_split=true))"]))
CONFIGS.append(common_setup.IssueConfig(f"daOTF-4000", ["--search", "astar(domain_abstraction(precalculation=false, max_states=4000, initial_goal_split=false))"]))
CONFIGS.append(common_setup.IssueConfig(f"pdbCEGAR-SinglePattern", ["--search", "astar(pdb(cegar_pattern(max_pdb_size=1000000,max_time=100,use_wildcard_plans=true,verbosity=normal,random_seed=2018)))"]))
CONFIGS.append(common_setup.IssueConfig(f"pdbCEGAR-additive", ["--search", "astar(cpdbs(disjoint_cegar(use_wildcard_plans=true,max_time=100,max_pdb_size=1000000,max_collection_size=10000000,random_seed=2018,verbosity=normal)),verbosity=normal)"]))
CONFIGS.append(common_setup.IssueConfig(f"pdbCEGAR-nonadditive", ["--search", "astar(cpdbs(multiple_cegar(max_pdb_size=1000000,max_collection_size=10000000,pattern_generation_max_time=infinity,total_max_time=100,stagnation_limit=20,blacklist_trigger_percentage=0.75,enable_blacklist_on_stagnation=true,random_seed=2018,verbosity=normal,use_wildcard_plans=false)),verbosity=silent)"]))
CONFIGS.append(common_setup.IssueConfig(f"CartesianCEGAR-SinglePattern", ["--search", "astar(cegar(subtasks=[original()], max_transitions=infinity,max_time=900))"]))

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

#exp.add_absolute_report_step(attributes=(["Num AbstractStates", "Num CEGAR Loop Iterations", "Precalculation-Time"] + common_setup.IssueExperiment.DEFAULT_TABLE_ATTRIBUTES))
#exp.add_report(ScatterPlotReport(attributes=["expansions_until_last_jump", "coverage", "total_time"], filter_algorithm=["daOTF-4000", "daPrecomp-1024-gs"], format="png", name="scatterplot-expansions"))
exp.add_onerev_scatter_plot_step()
exp.add_parse_again_step()

exp.run_steps()
