#include "domain_abstraction_heuristic.h"

#include "../task_utils/task_properties.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

#include <cstddef>
#include <limits>
#include <utility>


// Test run: ./fast-downward.py --debug misc/tests/benchmarks/gripper/prob01.pddl --search "astar(domain_abstraction())"

using namespace std;

namespace domain_abstractions {
    shared_ptr <HeuristicBasis>
    DomainAbstractionHeuristic::generate_heuristic(const Options &opts, utils::LogProxy &log) {
        if (log.is_at_least_normal()) {
            log << "Initializing Domain Abstraction heuristic using CEGAR-Like Algorithm..." << endl;
            task_properties::dump_goals(task_proxy.get_goals());
        }
        double max_time = opts.get<double>("max_time");
        int max_states = opts.get<int>("max_states");
        string splitMethod = opts.get<string>("split_method");
        shared_ptr<HeuristicBasis> h = make_shared<HeuristicBasis>(max_time, log, task_proxy, splitMethod);
        // call to construct will start refinement using CEGAR-Algorithm, check that no axioms and conditional effects!
        task_properties::verify_no_axioms(task_proxy);
        task_properties::verify_no_conditional_effects(task_proxy);
        h->construct(task_proxy);
        return h;
    }

    DomainAbstractionHeuristic::DomainAbstractionHeuristic(const options::Options &opts)
            : Heuristic(opts),
              heuristic_function(generate_heuristic(opts, log)) {
    }

    DomainAbstractionHeuristic::~DomainAbstractionHeuristic() = default;

    int DomainAbstractionHeuristic::compute_heuristic(const State &ancestor_state) {
        /*
         Computes the heuristic value by using the heuristic instances getValue() method
         */
        State state = convert_ancestor_state(ancestor_state);
        int value = heuristic_function->getValue(state);
        if (value == INF) {
            return DEAD_END;
        }
        assert(value >= 0);
        return value;
    }

    static shared_ptr <Heuristic> _parse(OptionParser &parser) {
        parser.document_synopsis("Domain Abstraction Heuristic",
                                 "Constructs an Abstraction heuristic using CEGAR-Like Algorithm based"
                                 "on Domain Abstractions");
        parser.add_option<int>(
                "max_states",
                "maximum sum of abstract states in the abstractions",
                "infinity",
                Bounds("1", "infinity"));
        parser.add_option<double>(
                "max_time",
                "maximum time in seconds for building abstractions",
                "infinity",
                Bounds("0.0", "infinity"));
        parser.add_option<string>("split_method",
                                  "The Method how the Abstraction Refinement works",
                                  "HardSplit");

        Heuristic::add_options_to_parser(parser);
        Options opts = parser.parse();
        if (parser.dry_run())
            return nullptr;
        else
            return make_shared<DomainAbstractionHeuristic>(opts);
    }

    static Plugin<Evaluator> _plugin("domain_abstraction", _parse);
}
