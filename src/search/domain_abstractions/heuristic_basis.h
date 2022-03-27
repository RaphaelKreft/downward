#ifndef FAST_DOWNWARD_DA_HEURISTIC_H
#define FAST_DOWNWARD_DA_HEURISTIC_H

#include "../task_proxy.h"
#include "../task_utils/task_properties.h"
#include "../tasks/modified_operator_costs_task.h"
#include "../utils/countdown_timer.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include "domain_abstraction.h"

#include <algorithm>
#include <cassert>
#include <vector>

// make State class from task_proxy module visible here
class State;

namespace domain_abstractions {

    class HeuristicBasis {
        double max_time;
        utils::LogProxy &log;

        DomainAbstraction abstraction;
        std::vector<std::vector<int>> HeuristicValues;
    public:
        explicit HeuristicBasis(int max_time, utils::LogProxy &log);
        int getValue(State state);
        void construct(std::shared_ptr<AbstractTask> originalTask);
    protected:
        bool shouldTerminate();
        void findOptimalTrace();
        void refine();
        void findFlaw(std::shared_ptr<AbstractTask> originalTask);
        void extractPath(std::shared_ptr<AbstractTask> originalTask));
        void trivialAbstraction(std::shared_ptr<AbstractTask> originalTask);
    };
}



#endif //FAST_DOWNWARD_DA_HEURISTIC_H
