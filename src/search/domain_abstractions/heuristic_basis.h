#ifndef FAST_DOWNWARD_DA_HEURISTIC_H
#define FAST_DOWNWARD_DA_HEURISTIC_H

#include "../task_proxy.h"
#include "../task_utils/task_properties.h"
#include "../tasks/modified_operator_costs_task.h"
#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include "domain_abstraction.h"
#include "data_structures.h"
#include "split.h"
#include <algorithm>
#include <cassert>
#include <vector>

// make State class from task_proxy module visible here
class State;

namespace domain_abstractions {

    class HeuristicBasis {
        double max_time;
        utils::CountdownTimer timer;
        utils::LogProxy &log;

        shared_ptr<TransitionSystem> transitionSystem;
        unique_ptr<DomainAbstraction> abstraction;
        vector<int> heuristicValues;
        DomainSplitter domainSplitter;
    public:
        explicit HeuristicBasis(int max_time, utils::LogProxy &log, TaskProxy originalTask, string splitMethod);
        int getValue(State state);
        void construct(TaskProxy originalTask);
    protected:
        unique_ptr<DomainAbstraction> createAbstraction(TaskProxy originalTask);
        vector<int> calculateHeuristicValues();

        bool cegarShouldTerminate();
        shared_ptr<Trace> cegarFindOptimalTrace(unique_ptr<DomainAbstraction> currentAbstraction);
        void cegarRefine(shared_ptr<Flaw> flaw, unique_ptr<DomainAbstraction> currentDomainAbstraction);
        shared_ptr<Flaw> cegarFindFlaw(shared_ptr<Trace> trace, TaskProxy originalTask);
        Solution cegarExtractPath(shared_ptr<Trace> trace, TaskProxy originalTask);
        unique_ptr<DomainAbstraction> cegarTrivialAbstraction(TaskProxy originalTask);
    };
}



#endif //FAST_DOWNWARD_DA_HEURISTIC_H
