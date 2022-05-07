#ifndef DOMAIN_ABSTRACTIONS_HEURISTIC_BASIS_H
#define DOMAIN_ABSTRACTIONS_HEURISTIC_BASIS_H

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
#include <queue>

// make State class from task_proxy module visible here
class State;

namespace domain_abstractions {

    class HeuristicBasis {
        double max_time;
        utils::LogProxy &log;

        std::shared_ptr<TransitionSystem> transitionSystem;
        std::shared_ptr<DomainAbstraction> abstraction;
        //std::vector<int> heuristicValues; TODO: Disabled because of on the fly computation
        DomainSplitter domainSplitter;
        utils::CountdownTimer timer;
    public:
        explicit HeuristicBasis(double max_time, utils::LogProxy &log, TaskProxy originalTask, const std::string& splitMethod);

        int getValue(const State& state);

        void construct(TaskProxy originalTask);

    protected:
        std::shared_ptr<DomainAbstraction> createAbstraction(TaskProxy originalTask);

        std::vector<int> calculateHeuristicValues();

        bool cegarShouldTerminate();

        std::shared_ptr<Trace> cegarFindOptimalTrace(const std::shared_ptr<DomainAbstraction>& currentAbstraction);

        void cegarRefine(const std::shared_ptr<Flaw>& flaw, const std::shared_ptr<DomainAbstraction>& currentDomainAbstraction);

        std::shared_ptr<Flaw> cegarFindFlaw(const std::shared_ptr<Trace>& trace, TaskProxy originalTask);

        std::shared_ptr<DomainAbstraction> cegarTrivialAbstraction(TaskProxy originalTask);

        int calculateHValueOnTheFly(const VariableGroupVector& startStateValues, int abstractStateIndex);
    };
}


#endif
