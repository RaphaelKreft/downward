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
        bool OTF; // Will the heuristic values be calculated on the fly
        double max_time;
        int max_states;
        utils::LogProxy &log;

        std::shared_ptr<TransitionSystem> transitionSystem;
        std::shared_ptr<DomainAbstraction> abstraction;
        std::map<long long, int> heuristicValues;
        DomainSplitter domainSplitter;
        utils::CountdownTimer timer;
        bool terminationFlag;
    public:
        explicit HeuristicBasis(bool PRECALC, double max_time, int max_states, utils::LogProxy &log, TaskProxy originalTask,
                                const std::string &splitMethod);

        int getValue(const State &state);

        void construct(TaskProxy originalTask);

    protected:
        std::shared_ptr<DomainAbstraction> createAbstraction(TaskProxy originalTask);

        void calculateHeuristicValues();

        bool cegarShouldTerminate();

        std::shared_ptr<Trace> cegarFindOptimalTrace(const std::shared_ptr<DomainAbstraction> &currentAbstraction);

        void cegarRefine(const std::shared_ptr<Flaw> &flaw,
                         const std::shared_ptr<DomainAbstraction> &currentDomainAbstraction);

        std::shared_ptr<Flaw> cegarFindFlaw(const std::shared_ptr<Trace> &trace);

        std::shared_ptr<DomainAbstraction> cegarTrivialAbstraction(TaskProxy originalTask);

        int calculateHValueOnTheFly(const VariableGroupVector &startStateValues, long long abstractStateIndex);
    };
}


#endif
